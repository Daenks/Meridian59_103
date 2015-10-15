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

// refers to the i-th element on the heap of a room
#define HEAP(r, i) ((r)->Astar.NodesData[i].heapslot)

bool AStarHeapCheck(room_type* Room, int i)
{
   int squares = Room->colshighres * Room->rowshighres;

   if (i >= squares)
      return true;

   if (!HEAP(Room, i))
      return true;

   float our = HEAP(Room, i)->Data->combined;
   int lchild = LCHILD(i);
   int rchild = RCHILD(i);

   if (lchild < squares)
   {
      astar_node* node = HEAP(Room, lchild);
      if (node)
      {
         if (node->Data->combined < our)
            return false;
      }
   }
   if (rchild < squares)
   {
      astar_node* node = HEAP(Room, rchild);
      if (node)
      {
         if (node->Data->combined < our)
            return false;
      }
   }

   bool retval = AStarHeapCheck(Room, lchild);
   if (!retval)
      return false;

   return AStarHeapCheck(Room, rchild);
}

void AStarWriteHeapToFile(room_type* Room)
{
   int rows = Room->rowshighres;
   int cols = Room->colshighres;
   char* rowstring = (char *)AllocateMemory(MALLOC_ID_ROOM, 60000);
   FILE *fp = fopen("heapdebug.txt", "a");
   if (fp)
   {
      int maxlayer = 9;	
      int treesize = Room->colshighres * Room->rowshighres;
      int rangestart = 0;
      int rangesize = 1;
      for (int i = 1; i < maxlayer; i++)
      {
         if (treesize < rangestart + rangesize)
            break;

         sprintf(rowstring, "L:%.2i", i);

         for (int k = 0; k < rangesize; k++)
         {				
            astar_node* node = Room->Astar.NodesData[rangestart + k].heapslot;

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
   FreeMemory(MALLOC_ID_ROOM, rowstring, 60000);
}

__inline void AStarHeapSwap(room_type* Room, int idx1, int idx2)
{
   astar_node* node1 = HEAP(Room, idx1);
   astar_node* node2 = HEAP(Room, idx2);	
   HEAP(Room, idx1) = node2;
   HEAP(Room, idx2) = node1;
   node1->Data->heapindex = idx2;
   node2->Data->heapindex = idx1;
}

__inline void AStarHeapMoveUp(room_type* Room, int Index)
{
   int i = Index;
   while (i > 0 && HEAP(Room, i)->Data->combined < HEAP(Room, PARENT(i))->Data->combined)
   {
      AStarHeapSwap(Room, i, PARENT(i));
      i = PARENT(i);
   }
}

__inline void AStarHeapHeapify(room_type* Room, int Index)
{
   int i = Index;
   do 
   {
      int min = i;
      if (HEAP(Room, LCHILD(i)) && HEAP(Room, LCHILD(i))->Data->combined < HEAP(Room, min)->Data->combined)
         min = LCHILD(i);
      if (HEAP(Room, RCHILD(i)) && HEAP(Room, RCHILD(i))->Data->combined < HEAP(Room, min)->Data->combined)
         min = RCHILD(i);
      if (min == i)
         break;
      AStarHeapSwap(Room, i, min);
      i = min;
   } 
   while (true);
}

__inline void AStarHeapInsert(room_type* Room, astar_node* Node)
{
   // save index of this node
   Node->Data->heapindex = Room->Astar.HeapSize;

   // add node at the end
   HEAP(Room, Node->Data->heapindex) = Node;

   // increment
   Room->Astar.HeapSize++;

   // push node up until in place
   AStarHeapMoveUp(Room, Node->Data->heapindex);
}

__inline void AStarHeapRemoveFirst(room_type* Room)
{
   int lastIdx = Room->Astar.HeapSize - 1;

   // no elements
   if (lastIdx < 0)
      return;

   // decrement size
   Room->Astar.HeapSize--;

   // more than 1
   if (lastIdx > 0)
   {
      // put last at root
      AStarHeapSwap(Room, 0, lastIdx);

      // zero out the previous root at swapped slot
      HEAP(Room, lastIdx)->Data->heapindex = 0;
      HEAP(Room, lastIdx) = NULL;

      // reorder tree
      AStarHeapHeapify(Room, 0);
   }

   // only one, clear head
   else
   {
      HEAP(Room, 0)->Data->heapindex = 0;
      HEAP(Room, 0) = NULL;
   }
}

__inline void AStarAddBlockers(room_type *Room)
{
   Blocker* b;
	
   b = Room->Blocker;
   while (b)
   {
      // Don't add self.
      if (b->ObjectID == Room->Astar.ObjectID)
      {
         b = b->Next;
         continue;
      }

      // Get blocker coords
      int row = (int)roundf(b->Position.Y / 256.0f);
      int col = (int)roundf(b->Position.X / 256.0f);
	  
      // Don't add blockers at the target coords.
      if (abs(row - Room->Astar.EndNode->Row) < DESTBLOCKIGNORE &&
          abs(col - Room->Astar.EndNode->Col) < DESTBLOCKIGNORE)
      {
         b = b->Next;
         continue;
      }

      // Mark these nodes in A* grid blocked (our coord and +2 highres each dir)
      for (int rowoffset = -2; rowoffset < 3; rowoffset++)
      {
         for (int coloffset = -2; coloffset < 3; coloffset++)
         {
            int r = row + rowoffset;
            int c = col + coloffset;

            // outside
            if (r < 0 || c < 0 || r >= Room->rowshighres || c >= Room->colshighres)
               continue;

            astar_node* node = &Room->Astar.Grid[r][c];
            node->Data->isBlocked = true;
         }
      }
      b = b->Next;
   }
}

__inline bool AStarProcessNode(room_type* Room)
{
   // shortcuts
   astar_node* node = HEAP(Room, 0);
   astar_node* endNode = Room->Astar.EndNode;

   Wall* blockWall;

   // openlist empty, unreachable
   if (!node)
      return false;

   // close enough, we're done, path found
   if (abs(node->Col - endNode->Col) < CLOSEENOUGHDIST &&
       abs(node->Row - endNode->Row) < CLOSEENOUGHDIST)
   {
      Room->Astar.LastNode = node;
      return false;
   }

   // these 9 loop-iterations will go over all 8 adjacent squares and the node itself
   // adjacent squares which are reachable will be added to the open-list with their costs
   for (int rowoffset = -1; rowoffset < 2; rowoffset++)
   {
      for (int coloffset = -1; coloffset < 2; coloffset++)
      {
         // skip evaluating ourself
         if (rowoffset == 0 && coloffset == 0)
            continue;

         // indices of node to process
         int r = node->Row + rowoffset;
         int c = node->Col + coloffset;

         // outside
         if (r < 0 || c < 0 || r >= Room->rowshighres || c >= Room->colshighres)
            continue;

         // get candidate node for indices
         astar_node* candidate = &Room->Astar.Grid[r][c];

         // skip any already examined, outside or blocked node
         if (candidate->Data->isInClosedList || !candidate->Leaf || candidate->Data->isBlocked)
            continue;

         // can't move from node to this candidate
         if (!BSPCanMoveInRoom(Room, &node->Location, &candidate->Location, Room->Astar.ObjectID, false, &blockWall, true))
            continue;
		 
         // cost for diagonal is sqrt(2), otherwise 1
         float stepcost = ((rowoffset != 0) && (coloffset != 0)) ? (float)M_SQRT2 : 1.0f;
			
         // heuristic (~ estimated distance from node to end)
         // need to do this only once
         if (candidate->Data->heuristic == 0.0f)
         {
            float dx = (float)abs(candidate->Col - endNode->Col);
            float dy = (float)abs(candidate->Row - endNode->Row);

            // octile-distance
            candidate->Data->heuristic = 
               HCOST * (dx + dy) + (HCOST_DIAG - 2.0f * HCOST) * fminf(dx, dy);

            // tie breaker and fixes h(nondiagonal) not lower exact cost
            candidate->Data->heuristic *= 0.999f; 
         }

         // CASE 1)
         // if this candidate has no parent yet (never visited before)
         if (!candidate->Data->parent)
         {
            // cost to candidate is cost to Node + one step
            candidate->Data->parent = node;
            candidate->Data->cost = node->Data->cost + stepcost;
            candidate->Data->combined = candidate->Data->cost + candidate->Data->heuristic;

            // add it sorted to the open list
            AStarHeapInsert(Room, candidate);
         }

         // CASE 2)
         // we already got a path to this candidate
         else
         {
            // our cost to the candidate
            float newcost = node->Data->cost + stepcost;

            // we're cheaper, so update the candidate
            // the real cost matters here, not including the heuristic
            if (newcost < candidate->Data->cost)
            {
               candidate->Data->parent = node;
               candidate->Data->cost = newcost;
               candidate->Data->combined = newcost + candidate->Data->heuristic;

               // reorder it upwards in the heap tree, don't care about downordering
               // since costs are lower and heuristic is always the same,
               // it's guaranteed to be moved up
               AStarHeapMoveUp(Room, candidate->Data->heapindex);
            }
         }
      }
   }

   /****************************************************************/

   // mark this node processed
   node->Data->isInClosedList = true;

   // remove it from the open list
   AStarHeapRemoveFirst(Room);

   return true;
}

void AStarWriteGridToFile(room_type* Room)
{
   int rows = Room->rowshighres;
   int cols = Room->colshighres;

   char *rowstring = (char *)AllocateMemory(MALLOC_ID_ROOM, 50000);

   FILE *fp = fopen("griddebug.txt", "w");
   if (fp)
   {
      for (int row = 0; row < rows; row++)
      {
         sprintf(rowstring, "Row %3i- ", row);
         for (int col = 0; col < cols; col++)
         {
            sprintf(rowstring, "%s|%7.3f|", rowstring, Room->Astar.Grid[row][col].Data->combined);
         }
         sprintf(rowstring, "%s \n", rowstring);
         fputs(rowstring, fp);
      }
      fclose(fp);
   }

   FreeMemory(MALLOC_ID_ROOM, rowstring, 50000);
}

void AStarGenerateGrid(room_type* Room)
{
   // note: we allocate all memory for the astar_node_data of all squares together
   //  this is necessary so we can erase it with a single cheap ZeroMemory call..
   Room->Astar.NodesDataSize = Room->colshighres * Room->rowshighres * sizeof(astar_node_data);

   // allocate memory for all nodesdata (cleaned for each calculation)
   Room->Astar.NodesData = (astar_node_data*)AllocateMemory(
      MALLOC_ID_ASTAR, Room->Astar.NodesDataSize);

   // allocate memory for the persistent nodesinfo (typical 2d array by **)
   Room->Astar.Grid = (astar_node**)AllocateMemory(
      MALLOC_ID_ASTAR, Room->rowshighres * sizeof(astar_node*));

   // setup rows
   for (int i = 0; i < Room->rowshighres; i++)	
      Room->Astar.Grid[i] = (astar_node*)AllocateMemory(
         MALLOC_ID_ASTAR, Room->colshighres * sizeof(astar_node));

   // setup all squares
   for (int i = 0; i < Room->rowshighres; i++)
   {
      for (int j = 0; j < Room->colshighres; j++)
      {
         astar_node* node = &Room->Astar.Grid[i][j];
         float f1, f2, f3;
         node->Row = i;
         node->Col = j;

         // floatingpoint coordinates of the center of the square in ROO fineness (for queries)
         node->Location.X = j * 256.0f;// +128.0f;
         node->Location.Y = i * 256.0f;// +128.0f;

         // mark if this square is inside or outside of room
         BSPGetHeight(Room, &node->Location, &f1, &f2, &f3, &node->Leaf);

         // setup reference to data (costs etc.) in erasable mem area
         node->Data = &Room->Astar.NodesData[i*Room->colshighres + j];
      }
   }
}

void AStarFreeGrid(room_type* Room)
{
   // free workdata mem
   FreeMemory(MALLOC_ID_ASTAR, Room->Astar.NodesData, Room->Astar.NodesDataSize);

   // free each row mem
   for (int i = 0; i < Room->rowshighres; i++)	
      FreeMemory(MALLOC_ID_ASTAR, Room->Astar.Grid[i], Room->colshighres * sizeof(astar_node));
	
   // free rowsmem
   FreeMemory(MALLOC_ID_ASTAR, Room->Astar.Grid, Room->rowshighres * sizeof(astar_node*));
}

bool AStarGetStepTowards(room_type* Room, V2* S, V2* E, V2* P, unsigned int* Flags, int ObjectID)
{
   // convert coordinates from ROO floatingpoint to
   // highres scale in integers
   int startrow = (int)roundf(S->Y / 256.0f);
   int startcol = (int)roundf(S->X / 256.0f);
   int endrow = (int)roundf(E->Y / 256.0f);
   int endcol = (int)roundf(E->X / 256.0f);

   // all 4 values must be within grid array bounds!
   if (startrow < 0 || startrow >= Room->rowshighres ||
       startcol < 0 || startcol >= Room->colshighres ||
       endrow < 0 || endrow >= Room->rowshighres ||
       endcol < 0 || endcol >= Room->colshighres)
         return false;

   /**********************************************************************/

   // get start and endnode
   astar_node* startnode = &Room->Astar.Grid[startrow][startcol];
   Room->Astar.EndNode = &Room->Astar.Grid[endrow][endcol];

   // init the astar struct
   Room->Astar.LastNode = NULL;
   Room->Astar.ObjectID = ObjectID;
   Room->Astar.HeapSize = 0;

   // prepare non-persistent astar grid data memory
   ZeroMemory(Room->Astar.NodesData, Room->Astar.NodesDataSize);

   /**********************************************************************/

   // mark nodes blocked by objects 
   AStarAddBlockers(Room);

   // insert startnode into heap-tree
   AStarHeapInsert(Room, startnode);

   // the algorithm finishes if we either hit a node close enough to endnode
   // or if there is no more entries in the open list (unreachable)
   while (AStarProcessNode(Room)) { }

   //AStarWriteGridToFile(Room);
   //AStarWriteHeapToFile(Room);
   /**********************************************************************/

   // now let's walk back our parent pointers starting from the LastNode
   // this chain represents the path (if it was unreachable, it's null)
   astar_node* parent = Room->Astar.LastNode;
   while (parent && parent->Data->parent != startnode)
      parent = parent->Data->parent;

   // unreachable
   if (!parent)
      return false;
	
   // for diagonal moves mark to be long step (required for timer elapse)
   if (abs(parent->Col - startnode->Col) &&
       abs(parent->Row - startnode->Row))
   {
      *Flags |= ESTATE_LONG_STEP;
   }
   else
      *Flags &= ~ESTATE_LONG_STEP;

   // set step endpoint
   *P = parent->Location;

   return true;
}
