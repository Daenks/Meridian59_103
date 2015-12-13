// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* astar.c
*

Improvements over the old implementation:
  * Allocation of gridmemory is only done once when room is loaded
  * Node information is split up between persistent data and data
    which needs to be cleared for each algorithm and can be by single ZeroMemory
  * Whether a node is in the closed set is not saved and looked up by an additional list,
    but rather simply saved by a flag on that node.
  * Uses a binary heap to store the open set
*/

#include "blakserv.h"

#define LCHILD(x) (2 * x + 1)
#define RCHILD(x) (2 * x + 2)
#define PARENT(x) ((x-1) / 2)

astar                 AStar[THREADSCOUNT];
astar_query_queue     AStarQueries = astar_query_queue(QUERYQUEUESIZE);
astar_response_queue  AStarResponses = astar_response_queue(RESPONSEQUEUESIZE);

void AStarThreadProc(astar* Astar);

#pragma region HEAP
// refers to the i-th element on the heap
#define HEAP(a, i) ((a)->NodesData[i].heapslot)

__inline bool AStarHeapCheck(astar* Astar, room_type* Room, int i)
{
   int squares = Room->rowshighres * Room->colshighres;

   if (i >= squares)
      return true;

   if (!HEAP(Astar, i))
      return true;

   float our = HEAP(Astar, i)->Data->combined;
   int lchild = LCHILD(i);
   int rchild = RCHILD(i);

   if (lchild < squares)
   {
      astar_node* node = HEAP(Astar, lchild);
      if (node)
      {
         if (node->Data->combined < our)
            return false;
      }
   }
   if (rchild < squares)
   {
      astar_node* node = HEAP(Astar, rchild);
      if (node)
      {
         if (node->Data->combined < our)
            return false;
      }
   }

   bool retval = AStarHeapCheck(Astar, Room, lchild);
   if (!retval)
      return false;

   return AStarHeapCheck(Astar, Room, rchild);
}

__inline void AStarWriteHeapToFile(astar* Astar, room_type* Room)
{
   int rows = Room->rowshighres;
   int cols = Room->colshighres;
   char* rowstring = (char *)AStarAlloc(Astar, 60000);
   FILE *fp = fopen("heapdebug.txt", "a");
   if (fp)
   {
      int maxlayer = 9;	
      int treesize = cols * rows;
      int rangestart = 0;
      int rangesize = 1;
      for (int i = 1; i < maxlayer; i++)
      {
         if (treesize < rangestart + rangesize)
            break;

         sprintf(rowstring, "L:%.2i", i);

         for (int k = 0; k < rangesize; k++)
         {				
            astar_node* node = Astar->NodesData[rangestart + k].heapslot;

            if (node)
               sprintf(rowstring, "%s|%6.2f|", rowstring, node->Data->combined);
            else
               sprintf(rowstring, "%s|XXXXXX|", rowstring);
         }

         sprintf(rowstring, "%s\n", rowstring);
         fputs(rowstring, fp);
			
         rangestart = rangestart + rangesize; 
         rangesize *= 2;		
      }
      sprintf(rowstring, "%s\n", rowstring);
      fclose(fp);
   }
   AStarFree(Astar, rowstring, 60000);
}

__inline void AStarHeapSwap(astar* Astar, int idx1, int idx2)
{
   astar_node* node1 = HEAP(Astar, idx1);
   astar_node* node2 = HEAP(Astar, idx2);	
   HEAP(Astar, idx1) = node2;
   HEAP(Astar, idx2) = node1;
   node1->Data->heapindex = idx2;
   node2->Data->heapindex = idx1;
}

__inline void AStarHeapMoveUp(astar* Astar, int Index)
{
   int i = Index;
   while (i > 0 && HEAP(Astar, i)->Data->combined < HEAP(Astar, PARENT(i))->Data->combined)
   {
      AStarHeapSwap(Astar, i, PARENT(i));
      i = PARENT(i);
   }
}

__inline void AStarHeapHeapify(astar* Astar, int Index)
{
   int i = Index;
   do 
   {
      int min = i;
      if (HEAP(Astar, LCHILD(i)) && HEAP(Astar, LCHILD(i))->Data->combined < HEAP(Astar, min)->Data->combined)
         min = LCHILD(i);
      if (HEAP(Astar, RCHILD(i)) && HEAP(Astar, RCHILD(i))->Data->combined < HEAP(Astar, min)->Data->combined)
         min = RCHILD(i);
      if (min == i)
         break;
      AStarHeapSwap(Astar, i, min);
      i = min;
   } 
   while (true);
}

__inline void AStarHeapInsert(astar* Astar, astar_node* Node)
{
   // save index of this node
   Node->Data->heapindex = Astar->HeapSize;

   // add node at the end
   HEAP(Astar, Node->Data->heapindex) = Node;

   // increment
   Astar->HeapSize++;

   // push node up until in place
   AStarHeapMoveUp(Astar, Node->Data->heapindex);
}

__inline void AStarHeapRemoveFirst(astar* Astar)
{
   int lastIdx = Astar->HeapSize - 1;

   // no elements
   if (lastIdx < 0)
      return;

   // decrement size
   Astar->HeapSize--;

   // more than 1
   if (lastIdx > 0)
   {
      // put last at root
      AStarHeapSwap(Astar, 0, lastIdx);

      // zero out the previous root at swapped slot
      HEAP(Astar, lastIdx)->Data->heapindex = 0;
      HEAP(Astar, lastIdx) = NULL;

      // reorder tree
      AStarHeapHeapify(Astar, 0);
   }

   // only one, clear head
   else
   {
      HEAP(Astar, 0)->Data->heapindex = 0;
      HEAP(Astar, 0) = NULL;
   }
}
#pragma endregion

#pragma region ASTAR 
__inline void AStarAddBlockers(astar* Astar)
{
   Blocker* b;
	
   b = Astar->Room->Room->Blocker;
   while (b)
   {
      // Don't add self.
      if (b->ObjectID == Astar->ObjectID)
      {
         b = b->Next;
         continue;
      }

      // Get blocker coords
      int row = (int)roundf(b->Position.Y / 256.0f);
      int col = (int)roundf(b->Position.X / 256.0f);
	  
      // Don't add blockers at the target coords.
      if (abs(row - Astar->EndNode->Meta->Row) < DESTBLOCKIGNORE &&
          abs(col - Astar->EndNode->Meta->Col) < DESTBLOCKIGNORE)
      {
         b = b->Next;
         continue;
      }

	  //todo: use neighbour[] ptrs here?
      // Mark these nodes in A* grid blocked (our coord and +2 highres each dir)
      for (int rowoffset = -2; rowoffset < 3; rowoffset++)
      {
         for (int coloffset = -2; coloffset < 3; coloffset++)
         {
            int r = row + rowoffset;
            int c = col + coloffset;

            // outside
            if (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres)
               continue;

            astar_node* node = &Astar->Room->Grid[r][c];
            node->Data->isBlocked = true;
         }
      }
      b = b->Next;
   }
}

__inline bool AStarCanWalkEdge(astar* Astar, astar_node* Node, astar_node* Neighbour, unsigned short KnowsVal, unsigned short CanVal)
{
   Wall* blockWall;

   bool knows = (((*Node->Edges) & KnowsVal) == KnowsVal);
   bool can;

   // not yet cached
   if (!knows)
   {
      // bsp query
      can = BSPCanMoveInRoom(Astar->Room->Room, &Node->Meta->Location, &Neighbour->Meta->Location, Astar->ObjectID, false, &blockWall, true);

      // save to cache
      *Node->Edges |= (can) ? CanVal : KnowsVal;
   }

   // retrieve answer from cache
   else
      can = (((*Node->Edges) & CanVal) == CanVal);

   return can;
}

__inline void AStarProcessNeighbour(astar* Astar, astar_node* Node, astar_node* Neighbour, float StepCost, unsigned short KnowsVal, unsigned short CanVal)
{
   // skip any neighbour outside of grid or sector, already processed or blocked
   if (!Neighbour || !Neighbour->Leaf || Neighbour->Data->isInClosedList || Neighbour->Data->isBlocked)
      return;

   // can't walk edge from node to neighbour
   if (!AStarCanWalkEdge(Astar, Node, Neighbour, KnowsVal, CanVal))
      return;

   // heuristic (~ estimated distance from node to end)
   // need to do this only once
   if (Neighbour->Data->heuristic == 0.0f)
   {
      float dx = (float)abs(Neighbour->Meta->Col - Astar->EndNode->Meta->Col);
      float dy = (float)abs(Neighbour->Meta->Row - Astar->EndNode->Meta->Row);

      // octile-distance
      Neighbour->Data->heuristic =
         COST * (dx + dy) + (COST_DIAG - 2.0f * COST) * fminf(dx, dy);

      // tie breaker and fixes h(nondiagonal) not lower exact cost
      Neighbour->Data->heuristic *= 0.999f;
   }

   // CASE 1)
   // if this candidate has no parent yet (never visited before)
   if (!Neighbour->Data->parent)
   {
      // cost to candidate is cost to Node + one step
      Neighbour->Data->parent = Node;
      Neighbour->Data->cost = Node->Data->cost + StepCost;
      Neighbour->Data->combined = Neighbour->Data->cost + Neighbour->Data->heuristic;

      // add it sorted to the open list
      AStarHeapInsert(Astar, Neighbour);
   }

   // CASE 2)
   // we already got a path to this candidate
   else
   {
      // our cost to the candidate
      float newcost = Node->Data->cost + StepCost;

      // we're cheaper, so update the candidate
      // the real cost matters here, not including the heuristic
      if (newcost < Neighbour->Data->cost)
      {
         Neighbour->Data->parent = Node;
         Neighbour->Data->cost = newcost;
         Neighbour->Data->combined = newcost + Neighbour->Data->heuristic;

         // reorder it upwards in the heap tree, don't care about downordering
         // since costs are lower and heuristic is always the same,
         // it's guaranteed to be moved up
         AStarHeapMoveUp(Astar, Neighbour->Data->heapindex);
      }
   }
}

__inline bool AStarProcessFirstOnHeap(astar* Astar)
{
   // shortcuts
   astar_node* node = HEAP(Astar, 0);
   astar_node* endNode = Astar->EndNode;

   // openlist empty, unreachable
   if (!node)
      return false;

   // close enough, we're done, path found
   if (abs(node->Meta->Col - endNode->Meta->Col) < CLOSEENOUGHDIST &&
       abs(node->Meta->Row - endNode->Meta->Row) < CLOSEENOUGHDIST)
   {
      Astar->LastNode = node;
      return false;
   }

   /****************************************************************/

   int r, c;
   astar_node* n;

   // straight neighbours

   // n
   r = node->Meta->Row - 1;
   c = node->Meta->Col + 0;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST, EDGECACHE_KNOWS_N, EDGECACHE_CAN_N);
   
   // e
   r = node->Meta->Row + 0;
   c = node->Meta->Col + 1;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST, EDGECACHE_KNOWS_E, EDGECACHE_CAN_E);
   
   // s
   r = node->Meta->Row + 1;
   c = node->Meta->Col + 0;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST, EDGECACHE_KNOWS_S, EDGECACHE_CAN_S);

   // w
   r = node->Meta->Row + 0;
   c = node->Meta->Col - 1;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST, EDGECACHE_KNOWS_W, EDGECACHE_CAN_W);

   // diagonal neighbours

   // ne
   r = node->Meta->Row - 1;
   c = node->Meta->Col + 1;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST_DIAG, EDGECACHE_KNOWS_NE, EDGECACHE_CAN_NE);

   // se
   r = node->Meta->Row + 1;
   c = node->Meta->Col + 1;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST_DIAG, EDGECACHE_KNOWS_SE, EDGECACHE_CAN_SE);

   // sw
   r = node->Meta->Row + 1;
   c = node->Meta->Col - 1;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST_DIAG, EDGECACHE_KNOWS_SW, EDGECACHE_CAN_SW);

   // nw
   r = node->Meta->Row - 1;
   c = node->Meta->Col - 1;
   n = (r < 0 || c < 0 || r >= Astar->Room->Room->rowshighres || c >= Astar->Room->Room->colshighres) ? NULL : &Astar->Room->Grid[r][c];
   AStarProcessNeighbour(Astar, node, n, COST_DIAG, EDGECACHE_KNOWS_NW, EDGECACHE_CAN_NW);

   /****************************************************************/

   // mark this node processed
   node->Data->isInClosedList = true;

   // remove it from the open list
   AStarHeapRemoveFirst(Astar);

   return true;
}

void AStarWriteGridToFile(astar_room* Room)
{
   int rows = Room->Room->rowshighres;
   int cols = Room->Room->colshighres;

   char *rowstring = (char *)malloc(50000);

   FILE *fp = fopen("griddebug.txt", "w");
   if (fp)
   {
      for (int row = 0; row < rows; row++)
      {
         sprintf(rowstring, "Row %3i- ", row);
         for (int col = 0; col < cols; col++)
         {
            sprintf(rowstring, "%s|%7.3f|", rowstring, Room->Grid[row][col].Data->combined);
         }
         sprintf(rowstring, "%s \n", rowstring);
         fputs(rowstring, fp);
      }
      fclose(fp);
   }

   free(rowstring);
}

void AStarInit()
{
   // start workers
   for (int i = 0; i < THREADSCOUNT; i++)
   {
      AStar[i].Commands = new astar_command_queue(COMMANDQUEUESIZE);
      AStar[i].Thread = new ::std::thread(AStarThreadProc, &AStar[i]);
   }
}

void AStarShutdown()
{
   // stop workers
   for (int i = 0; i < THREADSCOUNT; i++)  
      AStar[i].IsRunning.store(false);  
}
#pragma endregion

#pragma region THREADING
void AStarHandleLoadRoom(astar* Astar, astar_command_loadroom* Command)
{
   room_type* bsproom = (room_type*)AStarAlloc(Astar, sizeof(room_type));

   // failed to load room
   if (!BSPLoadRoom(Command->file, bsproom, Astar))
   {
      AStarFree(Astar, bsproom, sizeof(room_type));
      return;
   }

   // set id on our own instance
   bsproom->roomdata_id = Command->id;
   bsproom->resource_id = 0;

   // create astar room
   astar_room* room = (astar_room*)AStarAlloc(Astar, sizeof(astar_room));
   room->Room = bsproom;
	
   if (bsproom->colshighres > MAXGRIDCOLS || bsproom->rowshighres > MAXGRIDROWS)
      eprintf("ALERT!! You tried to load a room with a grid-size beyond allowed limits! AStar will fail.");

   /**********************************************************************/

   // allocate memory to cache edges (BSP queries)
   room->EdgesCacheSize = bsproom->colshighres * bsproom->rowshighres * sizeof(unsigned short);
   room->EdgesCache = (unsigned short*)AStarAlloc(Astar, room->EdgesCacheSize);
   ZeroMemory(room->EdgesCache, room->EdgesCacheSize);

   /**********************************************************************/

   // allocate memory for the persistent nodesinfo (typical 2d array by **)
   room->Grid = (astar_node**)AStarAlloc(Astar, bsproom->rowshighres * sizeof(astar_node*));

   // allocate rows
   for (int i = 0; i < bsproom->rowshighres; i++)
      room->Grid[i] = (astar_node*)AStarAlloc(Astar, bsproom->colshighres * sizeof(astar_node));

   /**********************************************************************/

   // setup all squares
   for (int i = 0; i < bsproom->rowshighres; i++)
   {
      int row = (i < MAXGRIDROWS) ? i : 0;

      for (int j = 0; j < bsproom->colshighres; j++)
      {
         int col = (j < MAXGRIDCOLS) ? j : 0;
         int idx1 = row*bsproom->colshighres + col;
         int idx2 = i*bsproom->colshighres + j;

         astar_node* node = &room->Grid[i][j];

         // setup reference to data (costs etc.) in erasable mem area
         node->Data = &Astar->NodesData[idx1];
         node->Meta = &Astar->Grid[row][col];

         // setup reference to edgesdata in edgescache
         node->Edges = &room->EdgesCache[idx2];

         float f1, f2, f3;

         // mark if this square is inside or outside of room
         BSPGetHeight(bsproom, &node->Meta->Location, &f1, &f2, &f3, &node->Leaf);
      }
   }

   /**********************************************************************/

   // add to map
   Astar->Rooms->insert(::std::pair<int, astar_room*>(Command->id, room));
}

void AStarHandleUnloadRoom(astar* Astar, astar_command_unloadroom* Command)
{
   // lookup room
   ::std::map<int, astar_room*>::iterator item =
      Astar->Rooms->find(Command->id);

   // not found
   if (item == Astar->Rooms->end())
      return;

   astar_room* astarroom = item->second;
   room_type* bsproom = item->second->Room;

   // free each row mem
   for (int i = 0; i < bsproom->rowshighres; i++)
      AStarFree(Astar, astarroom->Grid[i], bsproom->colshighres * sizeof(astar_node));

   // free rowsmem
   AStarFree(Astar, astarroom->Grid, bsproom->rowshighres * sizeof(astar_node*));

   // free edgescache mem
   AStarFree(Astar, astarroom->EdgesCache, astarroom->EdgesCacheSize);

   // free room
   BSPFreeRoom(bsproom, Astar);

   // remove from map
   Astar->Rooms->erase(item);
}

void AStarHandleBlockerAdd(astar* Astar, astar_command_blockeradd* Command)
{
   // lookup room
   ::std::map<int, astar_room*>::iterator item =
      Astar->Rooms->find(Command->roomid);

   // not found
   if (item == Astar->Rooms->end())
      return;

   // call BSP function on our room instance
   BSPBlockerAdd(
      item->second->Room,
      Command->objectid,
      &Command->p);
}

void AStarHandleBlockerRemove(astar* Astar, astar_command_blockerremove* Command)
{
   // lookup room
   ::std::map<int, astar_room*>::iterator item =
      Astar->Rooms->find(Command->roomid);

   // not found
   if (item == Astar->Rooms->end())
      return;

   // call BSP function on our room instance
   BSPBlockerRemove(
      item->second->Room,
      Command->objectid);
}

void AStarHandleBlockerMove(astar* Astar, astar_command_blockermove* Command)
{
   // lookup room
   ::std::map<int, astar_room*>::iterator item =
      Astar->Rooms->find(Command->roomid);

   // not found
   if (item == Astar->Rooms->end())
      return;

   // call BSP function on our room instance
   BSPBlockerMove(
      item->second->Room,
      Command->objectid,
      &Command->p);
}

void AStarHandleBlockerClear(astar* Astar, astar_command_blockerclear* Command)
{
   // lookup room
   ::std::map<int, astar_room*>::iterator item =
      Astar->Rooms->find(Command->roomid);

   // not found
   if (item == Astar->Rooms->end())
      return;

   // call BSP function on our room instance
   BSPBlockerClear(item->second->Room);
}

void AStarHandleMoveSector(astar* Astar, astar_command_movesector* Command)
{
   // lookup room
   ::std::map<int, astar_room*>::iterator item = 
      Astar->Rooms->find(Command->roomid);
	
   // not found
   if (item == Astar->Rooms->end())
      return;

   // call BSP function on our room instance
   BSPMoveSector(
      item->second->Room,
      Command->serverid, 
      Command->floor, 
      Command->height, 
      Command->speed);

   // invalidate edges cache
   ZeroMemory(item->second->EdgesCache, item->second->EdgesCacheSize);

   // path cache is only in main-instances
}

void AStarHandleChangeTexture(astar* Astar, astar_command_changetexture* Command)
{
   // lookup room
   ::std::map<int, astar_room*>::iterator item =
      Astar->Rooms->find(Command->roomid);

   // not found
   if (item == Astar->Rooms->end())
      return;

   // call BSP function on our room instance
   BSPChangeTexture(
      item->second->Room,
      Command->serverid,
      Command->newtexture,
      Command->flags);

   // invalidate edges cache
   ZeroMemory(item->second->EdgesCache, item->second->EdgesCacheSize);

   // path cache is only in main-instances
}

void AStarHandleCommand(astar* Astar, astar_command* Command)
{
   switch (Command->commandType())
   {
   case astar_command_type::AStarLoadRoom:
      AStarHandleLoadRoom(Astar, (astar_command_loadroom*)Command);
      break;
   case astar_command_type::AStarUnloadRoom:
      AStarHandleUnloadRoom(Astar, (astar_command_unloadroom*)Command);
      break;
   case astar_command_type::AStarBlockerAdd:
      AStarHandleBlockerAdd(Astar, (astar_command_blockeradd*)Command);
      break;
   case astar_command_type::AStarBlockerRemove:
      AStarHandleBlockerRemove(Astar, (astar_command_blockerremove*)Command);
      break;
   case astar_command_type::AStarBlockerMove:
      AStarHandleBlockerMove(Astar, (astar_command_blockermove*)Command);
      break;
   case astar_command_type::AStarBlockerClear:
      AStarHandleBlockerClear(Astar, (astar_command_blockerclear*)Command);
      break;
   case astar_command_type::AStarMoveSector:
      AStarHandleMoveSector(Astar, (astar_command_movesector*)Command);
      break;
   case astar_command_type::AStarChangeTexture:
      AStarHandleChangeTexture(Astar, (astar_command_changetexture*)Command);
      break;
   }
   delete Command;
}

astar_pathresponse* AStarRunPathQuery(astar* Astar, astar_pathquery* Query)
{
   // return value
   astar_pathresponse* response = 
      new astar_pathresponse(Query->roomid, Query->objectid, Query->s, Query->e, Query->flags);

   // lookup room
   ::std::map<int, astar_room*>::iterator item =
      Astar->Rooms->find(Query->roomid);

   // not found
   if (item == Astar->Rooms->end())
      return response;

   astar_room* room = item->second;
   room_type* bsproom = room->Room;

   /**********************************************************************/

   // convert coordinates from ROO floatingpoint to
   // highres scale in integers
   int startrow = (int)roundf(Query->s.Y / 256.0f);
   int startcol = (int)roundf(Query->s.X / 256.0f);
   int endrow = (int)roundf(Query->e.Y / 256.0f);
   int endcol = (int)roundf(Query->e.X / 256.0f);

   // all 4 values must be within grid array bounds!
   if (startrow < 0 || startrow >= bsproom->rowshighres ||
       startcol < 0 || startcol >= bsproom->colshighres ||
       endrow < 0 || endrow >= bsproom->rowshighres ||
       endcol < 0 || endcol >= bsproom->colshighres)
       return response;

   /**********************************************************************/

   // set data on astar
   Astar->StartNode = &room->Grid[startrow][startcol];
   Astar->EndNode = &room->Grid[endrow][endcol];
   Astar->Room = room;
   Astar->LastNode = NULL;
   Astar->ObjectID = Query->objectid;
   Astar->HeapSize = 0;

   /**********************************************************************/

   // prepare non-persistent astar grid data memory
   ZeroMemory(Astar->NodesData, Astar->NodesDataSize);
   //ZeroMemory(Astar->NodesData, bsproom->rowshighres * bsproom->colshighres * sizeof(astar_node_data));

   /**********************************************************************/

   // mark nodes blocked by objects 
   AStarAddBlockers(Astar);

   // insert startnode into heap-tree
   AStarHeapInsert(Astar, Astar->StartNode);

   // the algorithm finishes if we either hit a node close enough to endnode
   // or if there is no more entries in the open list (unreachable)
   while (AStarProcessFirstOnHeap(Astar)) {}

   //AStarWriteGridToFile(Room);
   //AStarWriteHeapToFile(Room);

   /**********************************************************************/

   // unreachable
   if (!Astar->LastNode)
      return response;

   // walk back parent pointers from lastnode (=path)
   astar_node* node = Astar->LastNode;
   while (node)
   {
      // create path node (use malloc, "not our mem", will be used and freed in mainthread)
      astar_path_node* pathnode = (astar_path_node*)malloc(sizeof(astar_path_node));

      // set values
      pathnode->Row = node->Meta->Row;
      pathnode->Col = node->Meta->Col;
      pathnode->Location = node->Meta->Location;

      // add to path
      response->path.push_front(pathnode);

      // process next node
      node = node->Data->parent;
   }

   return response;
}

astar_response* AStarRunQuery(astar* Astar, astar_query* Query)
{
   astar_response* ret = NULL;
   switch (Query->queryType())
   {
   case astar_query_type::AStarPathQuery:
      ret = AStarRunPathQuery(Astar, (astar_pathquery*)Query);
      break;
   }
   return ret;
}

void AStarThreadProc(astar* Astar)
{
   Astar->IsRunning = true;
   Astar->IsPaused = false;
   Astar->DoPause = false;
   Astar->Memory = sizeof(astar);

   /**************************************************************************************/
   /*                                STARTUP                                             */
   /**************************************************************************************/

   // memory for astar data erased with each query
   Astar->NodesDataSize = NODESDATASQUARES * sizeof(astar_node_data);
   Astar->NodesData = (astar_node_data*)AStarAlloc(Astar, Astar->NodesDataSize);

   // basic init values
   Astar->StartNode = NULL;
   Astar->EndNode   = NULL;
   Astar->LastNode  = NULL;
   Astar->ObjectID  = 0;
   Astar->HeapSize  = 0;
   Astar->Room      = NULL;

   // create rooms list
   Astar->Rooms = new astar_rooms();

   // allocate memory for the persistent grid
   Astar->Grid = (astar_node_meta**)AStarAlloc(Astar, MAXGRIDROWS * sizeof(astar_node_meta*));

   // allocate rows
   for (int i = 0; i < MAXGRIDROWS; i++)
      Astar->Grid[i] = (astar_node_meta*)AStarAlloc(Astar, MAXGRIDCOLS * sizeof(astar_node_meta));

   // setup all squares
   for (int i = 0; i < MAXGRIDROWS; i++)
   {
      for (int j = 0; j < MAXGRIDCOLS; j++)
      {
         int idx = i*MAXGRIDCOLS + j;

         astar_node_meta* node = &Astar->Grid[i][j];

         // set row/col
         node->Row = i;
         node->Col = j;

         // floatingpoint coordinates of the center of the square in ROO fineness (for queries)
         node->Location.X = j * 256.0f;// +128.0f;
         node->Location.Y = i * 256.0f;// +128.0f;
      }
   }

   /**************************************************************************************/
   /*                                WORKING                                             */
   /**************************************************************************************/

   while (Astar->IsRunning.load())
   {
      // paused mode
      if (Astar->DoPause)
      {
         // sleep if pause is active
         if (Astar->IsPaused)
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(WORKERSLEEPTIMEMS));

         // go to pause-mode
         else
         {
            // handle all remaining commands
            while (astar_command* cmd = Astar->Commands->dequeue())
               AStarHandleCommand(Astar, cmd);

            Astar->IsPaused = true;
         }
      }

      // normal mode
      else
      {
         // make sure pause flag is unset
         Astar->IsPaused = false;

         // handle all pending commands
         while (astar_command* cmd = Astar->Commands->dequeue())
            AStarHandleCommand(Astar, cmd);

         // try to grab the next query-task
         astar_query* query = AStarQueries.dequeue();

         // if we got one, process it and enqueue the possible response
         if (query)
         {
            astar_response* response = AStarRunQuery(Astar, query);

            if (response)
               AStarResponses.enqueue(response);

            delete query;

            // let mainthread know about a new path being ready
            PostThreadMessage(main_thread_id, WM_BLAK_MAIN_PATH_READY, 0, 0);
         }

         // if there was no query to process, sleep a bit
         else		   		   
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(WORKERSLEEPTIMEMS));		 
      }	
   }

   /**************************************************************************************/
   /*                                SHUTDOWN                                            */
   /**************************************************************************************/

   AStarFree(Astar, Astar->NodesData, Astar->NodesDataSize);

   // free each meta grid row
   for (int i = 0; i < MAXGRIDROWS; i++)
      AStarFree(Astar, Astar->Grid[i], MAXGRIDCOLS * sizeof(astar_node_meta));

   // free meta grid
   AStarFree(Astar, Astar->Grid, MAXGRIDROWS * sizeof(astar_node_meta*));

   // delete rooms (todo: need cleanup of items, but blakserv is likely shutting down..)
   delete Astar->Rooms;
}

void AStarDeliverPathResponse(astar_pathresponse* Response)
{
   room_node* roomnode = GetRoomDataByID(Response->roomid);
   V2 p = { 0.0f, 0.0f };
   Wall* blockWall = NULL;
   
   if (!roomnode)
      return;
	
   // unreachable
   if (Response->path.size() == 0)
   {
      /*******************************************************************/
      // Add to nopath cache
      /*******************************************************************/
      // check for resetting nextpathidx
      if (roomnode->data.NextNoPathIdx >= NOPATHCACHESIZE)
         roomnode->data.NextNoPathIdx = 0;

      // get nopath instance from cache
      astar_nopath* nopath = roomnode->data.NoPaths[roomnode->data.NextNoPathIdx];
      roomnode->data.NextNoPathIdx++;

      // update values
      nopath->S = Response->s;
      nopath->E = Response->e;
      nopath->Tick = ::std::chrono::high_resolution_clock::now();

      // use heuristic
      if (BSPGetStepFromHeuristic(&roomnode->data, &Response->s, &Response->e, &p, &Response->flags, Response->objectid))
         BSPInvokeMoveCallback(Response->objectid, MC_HEURISTIC, Response->flags, &p);

      // fully blocked
      else
         BSPInvokeMoveCallback(Response->objectid, MC_UNREACHABLE, Response->flags, &p);
   }
   else
   {
      /*******************************************************************/
      // Add to path cache
      /*******************************************************************/

      // check for resetting nextpathidx
      if (roomnode->data.NextPathIdx >= PATHCACHESIZE)
         roomnode->data.NextPathIdx = 0;

      // get path from cache and clear it
      astar_path* path = roomnode->data.Paths[roomnode->data.NextPathIdx];
      roomnode->data.NextPathIdx++;

      // clear old cached path, use instances in response in our path
      BSPClearPath(path);
      path->assign(Response->path.begin(), Response->path.end());

      /*******************************************************************/
      // Deliver next step to kod object by objectid
      /*******************************************************************/
      
      // unset possible heuristic flags from earlier (unreachable) steps
      Response->flags &= ~MF_AVOIDING;
      Response->flags &= ~MF_CLOCKWISE;

      // try to do step suggested from path
      if (BSPGetStepFromPath(&roomnode->data, path, &path->front()->Location, &path->back()->Location, &p, &Response->flags, Response->objectid))
         BSPInvokeMoveCallback(Response->objectid, MC_NEWPATH, Response->flags, &p);

      // use heuristic
      else if (BSPGetStepFromHeuristic(&roomnode->data, &Response->s, &Response->e, &p, &Response->flags, Response->objectid))
         BSPInvokeMoveCallback(Response->objectid, MC_HEURISTIC, Response->flags, &p);

      // fully blocked
      else
         BSPInvokeMoveCallback(Response->objectid, MC_UNREACHABLE, Response->flags, &p);
   }
}

bool AStarDeliverNextResponse()
{
   astar_response* response = AStarResponses.dequeue();

   if (!response)
      return false;

   switch (response->responseType())
   {
   case astar_response_type::AStarPathResponse:
      AStarDeliverPathResponse((astar_pathresponse*)response);
      break;
   }

   delete response;
   return true;
}

void AStarEnqueueCommand(astar_command* Command)
{
   // send copy of command to each worker
   for (int i = 0; i < THREADSCOUNT; i++)
      AStar[i].Commands->enqueue(Command->clone());

   // delete the template
   delete Command;
}

void AStarInitGC()
{
   // tell workers to pause
   for (int i = 0; i < THREADSCOUNT; i++)
      AStar[i].DoPause = true;

   // wait for workers to get in pause mode
   // they will process all pending commands first
   while (true)
   {
      bool allpaused = true;

      for (int i = 0; i < THREADSCOUNT; i++)
         if (!AStar[i].IsPaused)
            allpaused = false;

      if (allpaused)
         break;
   }

   // clear the query queue, just discard them due to
   // invalid (ids) and monsters get their waiting flag reset on GC
   while (astar_query* query = AStarQueries.dequeue())
      delete query;
   
   // clear the response queues, free mem of pathnodes we not gonna use
   while (astar_response* response = AStarResponses.dequeue())
   {
      if (response->responseType() == AStarPathResponse)
      {
         astar_pathresponse* pathresponse = (astar_pathresponse*)response;
         while (!pathresponse->path.empty())
         {
            astar_path_node* node = pathresponse->path.front();
            pathresponse->path.pop_front();
            free(node);          
         }
      }
      delete response;
   }
}

void AStarFinishGC()
{
   // tell workers to run again
   for (int i = 0; i < THREADSCOUNT; i++)
      AStar[i].DoPause = false;
}

void AStarPerformGC()
{
   // loop through all workers
   for (int i = 0; i < THREADSCOUNT; i++)
   {
      astar* a = &AStar[i];

      // loop through all rooms of this worker
      for (std::map<int, astar_room*>::iterator it = a->Rooms->begin(); it != a->Rooms->end(); ++it)
      {
         room_type* bsproom = it->second->Room;
         Blocker* b = bsproom->Blocker;

         // loop through all blockers and update id
         while (b)
         {
            b->ObjectID = GetObjectByID(b->ObjectID)->garbage_ref;
            b = b->Next;
         }
      }
   }
}
#pragma endregion