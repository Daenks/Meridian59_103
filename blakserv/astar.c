// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* roofile.c
*

Improvements over the old implementation:
  * Allocation of gridmemory is only done once when room is loaded
  * Node information is split up between persistent data and data
    which needs to be cleared for each algorithm and can be by single ZeroMemory
  * Whether a node is in the open or closed set is not saved and looked up by an additional list,
    but rather simply saved by a flag on that node.
*/

#include "blakserv.h"

__inline void AStarOpenListInsert(room_type* Room, astar_node* Node)
{
   // add it sorted by lowest cost first to the open list
   astar_node* node = Room->Astar.Open;
   astar_node* prev = NULL;
   while (node && ((node->Data->cost + node->Data->heuristic) < (Node->Data->cost + Node->Data->heuristic)))
   {
      prev = node;
      node = node->Data->nextopen;
   }

   if (!prev)
   {
      Node->Data->nextopen = Room->Astar.Open;
      Room->Astar.Open = Node;
   }
   else
   {
      // insert
      Node->Data->nextopen = prev->Data->nextopen;
      prev->Data->nextopen = Node;
   }
}

__inline void AStarOpenListRemove(room_type* Room, astar_node* Node)
{
   astar_node* node = Room->Astar.Open;
   astar_node* prev = NULL;
   while (node)
   {
      if (node == Node)
      {
         // removed not first -> link previous to next one
         if (prev)
            prev->Data->nextopen = node->Data->nextopen;

         // removed first -> set listhead to next
         else
            Room->Astar.Open = node->Data->nextopen;

         return;
      }

      prev = node;
      node = node->Data->nextopen;
   }
}

__inline bool AStarProcessNode(room_type* Room)
{
   // shortcuts
   astar_node* Node = Room->Astar.Open;
   astar_node* EndNode = Room->Astar.EndNode;

   // openlist empty, unreachable
   if (!Node)
      return false;

   // close enough, we're done, path found
   if (abs(Node->Col - EndNode->Col) < CLOSEENOUGHDIST &&
       abs(Node->Row - EndNode->Row) < CLOSEENOUGHDIST)
   {
      Room->Astar.LastNode = Node;
      return false;
   }

   // these 9 loop-iterations will go over all 8 adjacent squares and the node itself
   // adjacent squares which are reachable will be added to the open-list with their costs
   for (int rowoffset = -1; rowoffset < 2; rowoffset++)
   {
      for (int coloffset = -1; coloffset < 2; coloffset++)
      {
         // indices of node to process
         int r = Node->Row + rowoffset;
         int c = Node->Col + coloffset;

         // outside
         if (r < 0 || c < 0 || r >= Room->rowshighres || c >= Room->colshighres)
            continue;

         // get candidate node for indices
         astar_node* candidate = &Room->Astar.Grid[r][c];

         // skip if ourself and any other already examined
         if (candidate == Node || candidate->Data->isInClosedList)
            continue;

         // can't move from node to this candidate
         if (!BSPCanMoveInRoom(Room, &Node->Location, &candidate->Location, Room->Astar.ObjectID, false))
            continue;

         // cost for diagonal is sqrt(2), otherwise 1
         float stepcost = (abs(rowoffset) && abs(coloffset)) ? (float)M_SQRT2 : 1.0f;
			
         // heuristic (~ estimated distance from node to end)
         // need to do this only once
         if (candidate->Data->heuristic == 0.0f)
         {
            float dx = fabs((float)candidate->Col - (float)EndNode->Col);
            float dy = fabs((float)candidate->Row - (float)EndNode->Row);
            candidate->Data->heuristic = 1.0f * (dx + dy) + ((float)M_SQRT2 - 2.0f * 1.0f) * fminf(dx, dy);
         }

         // CASE 1)
         // if this candidate has no parent yet (never visited before)
         if (!candidate->Data->parent)
         {
            // set Node as parent of candidate
            candidate->Data->parent = Node;

            // cost to candidate is cost to Node + one step
            candidate->Data->cost = Node->Data->cost + stepcost;
				
            // add it sorted to the open list
            AStarOpenListInsert(Room, candidate);
         }

         // CASE 2)
         // we already got a path to this candidate
         else
         {
            // our cost to the candidate
            float newcost = Node->Data->cost + stepcost;

            // we're cheaper, so update the candidate
            if (newcost < candidate->Data->cost)
            {
               candidate->Data->parent = Node;
               candidate->Data->cost = newcost;

               // remove it and add it to possibly new sorted index
               AStarOpenListRemove(Room, candidate);
               AStarOpenListInsert(Room, candidate);
            }
         }
      }
   }

   /****************************************************************/

   // mark this node processed
   Node->Data->isInClosedList = true;

   // remove it from the open list
   AStarOpenListRemove(Room, Node);

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
            sprintf(rowstring, "%s|%6.2f|", rowstring, Room->Astar.Grid[row][col].Data->cost);
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
         Room->Astar.Grid[i][j].Row = i;
         Room->Astar.Grid[i][j].Col = j;

         // floatingpoint coordinates of the center of the square in ROO fineness (for queries)
         Room->Astar.Grid[i][j].Location.X = j * 256.0f;// +128.0f;
         Room->Astar.Grid[i][j].Location.Y = i * 256.0f;// +128.0f;

         // setup reference to data (costs etc.) in erasable mem area 
         Room->Astar.Grid[i][j].Data = &Room->Astar.NodesData[i*Room->colshighres + j];
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

   // init the openlist with the startnode
   // and set the LastNode to NULL
   Room->Astar.Open = startnode;
   Room->Astar.LastNode = NULL;
   Room->Astar.ObjectID = ObjectID;

   // prepare non-persistent astar grid data memory
   ZeroMemory(Room->Astar.NodesData, Room->Astar.NodesDataSize);

   /**********************************************************************/

   // the algorithm finishes if we either hit a node close enough to endnode
   // or if there is no more entries in the open list (unreachable)
   while (AStarProcessNode(Room)) { }

   //AStarWriteGridToFile(Room);

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
