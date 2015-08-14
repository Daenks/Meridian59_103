// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * roomdata.c
 *
 This module maintains an array of pointers to room_node (all the room data)
 and also an array of pointers to room_rsc_node (used for fast room ID lookup
 by resource id). The arrays have a hardcoded size of 400, and each element
 is a linked list of pointers indexed by room_id % 400 for room_nodes, and
 resource_id % 400 for room_rsc_nodes. This allows a much smaller size to be
 set for the array, but can still handle an unforseen case where more than
 400 rooms get created. The hardcoded size (INIT_ROOMTABLE_SIZE in roomdata.h)
 should be increased if the 'normal' number of rooms on the server is ever over
 400 (currently ~270). .roo files loaded by the C function LoadRoom(), called
 from blakod.

 */

#include "blakserv.h"

// Next available room ID.
int           idcounter = 0;
// Array of pointers for room data storage.
room_node     **rooms;
// Array of pointers for room id lookup by resource.
room_rsc_node **room_rscs;

void InitRooms()
{
   rooms = (room_node **)AllocateMemoryCalloc(MALLOC_ID_ROOM,
      INIT_ROOMTABLE_SIZE, sizeof(room_node*));
   room_rscs = (room_rsc_node **)AllocateMemoryCalloc(MALLOC_ID_ROOM, 
      INIT_ROOMTABLE_SIZE, sizeof(room_rsc_node*));
   idcounter = 0;
}

void ExitRooms()
{
   ResetRooms();
   FreeMemory(MALLOC_ID_ROOM, rooms, INIT_ROOMTABLE_SIZE * sizeof(room_node*));
   FreeMemory(MALLOC_ID_ROOM, room_rscs, INIT_ROOMTABLE_SIZE * sizeof(room_rsc_node*));
}

void ResetRooms()
{
   room_node *room, *temp;
   room_rsc_node *rrsc, *rrscTemp;

   for (int i = 0; i < INIT_ROOMTABLE_SIZE; ++i)
   {
      if (room_rscs)
      {
         // Free memory from room_rscs first.
         rrsc = room_rscs[i];
         while (rrsc != NULL)
         {
            rrscTemp = rrsc->next;
            FreeMemory(MALLOC_ID_ROOM, rrsc, sizeof(room_rsc_node));
            rrsc = rrscTemp;
         }
         room_rscs[i % INIT_ROOMTABLE_SIZE] = NULL;
      }

      if (rooms)
      {
         // Free memory from rooms.
         room = rooms[i % INIT_ROOMTABLE_SIZE];
         while (room)
         {
            temp = room->next;
            BSPFreeRoom(&room->data);
            FreeMemory(MALLOC_ID_ROOM, room, sizeof(room_node));
            room = temp;
         }
         rooms[i % INIT_ROOMTABLE_SIZE] = NULL;
      }
   }

   idcounter = 0;
}

int LoadRoom(int resource_id)
{
   val_type ret_val;
   resource_node* r;
   room_rsc_node *rrsc;
   char s[MAX_PATH + FILENAME_MAX];

   /****************************************************************/

   r = GetResourceByID(resource_id);
   if (!r)
   {
      bprintf("LoadRoomData can't find resource %i\n",resource_id);
      return NIL;
   }

   ret_val.v.tag = TAG_ROOM_DATA;

   /****************************************************************/

   // CASE 1: Reuse already loaded ROO
   if (room_rscs)
   {
      rrsc = room_rscs[resource_id % INIT_ROOMTABLE_SIZE];
      while (rrsc)
      {
         if (rrsc->resource_id == resource_id)
         {
            ret_val.v.data = rrsc->roomdata_id;
            return ret_val.int_val;
         }
         rrsc = rrsc->next;
      }
   }

   /****************************************************************/

   // CASE 2: Load ROO
   room_node* newnode = (room_node*)AllocateMemory(MALLOC_ID_ROOM, sizeof(room_node));

   // combine path for roo and filename
   sprintf(s, "%s%s", ConfigStr(PATH_ROOMS), r->resource_val[0]);

   // try load it
   if (!BSPLoadRoom(s, &newnode->data))
   {
      FreeMemory(MALLOC_ID_ROOM, newnode, sizeof(room_node));
      bprintf("LoadRoomData couldn't open %s!!!\n",r->resource_val[0]);
      return NIL;
   }

   // Add this room_node to the rooms table.
   newnode->data.roomdata_id = idcounter++;
   newnode->data.resource_id = resource_id;
   newnode->next = rooms[newnode->data.roomdata_id % INIT_ROOMTABLE_SIZE];
   rooms[newnode->data.roomdata_id % INIT_ROOMTABLE_SIZE] = newnode;

   // Add room_rsc_node for this resource.
   rrsc = (room_rsc_node *)AllocateMemory(MALLOC_ID_ROOM, sizeof(room_rsc_node));

   rrsc->resource_id = resource_id;
   rrsc->roomdata_id = newnode->data.roomdata_id;
   rrsc->next = room_rscs[resource_id % INIT_ROOMTABLE_SIZE];
   room_rscs[resource_id % INIT_ROOMTABLE_SIZE] = rrsc;

   ret_val.v.data = newnode->data.roomdata_id;
   return ret_val.int_val;
}

void UnloadRoom(room_node *r)
{
   room_node *room, *temp;
   room_rsc_node *rrsc, *rrscTemp;

   if (!r)
   {
      bprintf("UnloadRoomData called with NULL room!");

      return;
   }

   if (room_rscs)
   {
      // Free the room_rsc_node data for this room.
      rrsc = room_rscs[r->data.resource_id % INIT_ROOMTABLE_SIZE];
      while (rrsc)
      {
         rrscTemp = rrsc->next;
         if (rrsc->roomdata_id == r->data.roomdata_id)
         {
            FreeMemory(MALLOC_ID_ROOM, rrsc, sizeof(room_rsc_node));
            rrsc = rrscTemp;
            room_rscs[r->data.resource_id % INIT_ROOMTABLE_SIZE] = rrscTemp;
            break;
         }
         rrsc = rrsc->next;
      }
   }

   if (rooms)
   {
      // Free the room.
      room = rooms[r->data.roomdata_id % INIT_ROOMTABLE_SIZE];
      while (room)
      {
         temp = room->next;
         if (room->data.roomdata_id == r->data.roomdata_id)
         {
            BSPFreeRoom(&room->data);
            room = temp;
            rooms[r->data.roomdata_id % INIT_ROOMTABLE_SIZE] = temp;
            return;
         }
         room = room->next;
      }
   }

   // If we get to this point, we didn't find the room we wanted to unload.
   bprintf("Room %i not freed in UnloadRoomData!", r->data.roomdata_id);

   return;
}

room_node * GetRoomDataByID(int id)
{
   room_node *room;

   if (!rooms)
      return NULL;

   room = rooms[id % INIT_ROOMTABLE_SIZE];
   while (room)
   {
      if (room->data.roomdata_id == id)
         return room;
      room = room->next;
   }

   return NULL;
}

room_node* GetRoomDataByResourceID(int id)
{
   room_rsc_node *rsc_node;

   if (!room_rscs)
      return NULL;

   rsc_node = room_rscs[id % INIT_ROOMTABLE_SIZE];
   while (rsc_node)
   {
      if (rsc_node->resource_id == id)
         return GetRoomDataByID(rsc_node->roomdata_id);
      rsc_node = rsc_node->next;
   }

   return NULL;
}

void ForEachRoom(void(*callback_func)(room_node *r))
{
   room_node *node;

   for (int i = 0; i < INIT_ROOMTABLE_SIZE; i++)
   {
      node = rooms[i];
      while (node)
      {
         callback_func(node);
         node = node->next;
      }
   }
}

int GetHeight(roomdata_node *r,int row,int col,int finerow,int finecol)
{
	int highresrow,highrescol;
	
	// no room or no heightmap
	if (!r || r->file_info.highres_grid == NULL)
		return 0;

	// build a combined value in fine precision first
	// then scale it to highres precision
	// formulas see in CanMoveInRoomHighRes
	highresrow = ((row << 6) + finerow) >> 4;
	highrescol = ((col << 6) + finecol) >> 4;
	
	// outside
	if (highresrow >= r->file_info.rowshighres ||
		highrescol >= r->file_info.colshighres)
		return 0;

	// return height from upper 23 bit
	return (r->file_info.highres_grid[highresrow][highrescol] >> 9);
}


int GetHeightFloorBSP(roomdata_node *r, int row, int col, int finerow, int finecol)
{
   if (!r || r->file_info.TreeNodesCount == 0)
      return 0;

   V2 p;
   p.X = FINENESSKODTOROO((float)col * 64.0f + (float)finecol);
   p.Y = FINENESSKODTOROO((float)row * 64.0f + (float)finerow);

   // get floor height (1:1024) with depth modifier
   // (=height of 'object's feet' in water - BELOW water texture)
   float height = BSPGetHeight(&r->file_info, &p, true, true);

   // height of -MIN_KOD_INT indicates a location outside of the map
   // leave this value untouched, otherwise scale from ROO FINENESS to KOD
   // and box value into min/max kod integer
   return (height == (float)-MIN_KOD_INT ? -MIN_KOD_INT : FLOATTOKODINT(FINENESSROOTOKOD(height)));
}

int GetHeightCeilingBSP(roomdata_node *r, int row, int col, int finerow, int finecol)
{
   if (!r || r->file_info.TreeNodesCount == 0)
      return 0;

   V2 p;
   p.X = FINENESSKODTOROO((float)col * 64.0f + (float)finecol);
   p.Y = FINENESSKODTOROO((float)row * 64.0f + (float)finerow);

   // get ceiling height (1:1024)
   float height = BSPGetHeight(&r->file_info, &p, false, false);

   // height of -MIN_KOD_INT indicates a location outside of the map
   // leave this value untouched, otherwise scale from ROO FINENESS to KOD
   // and box value into min/max kod integer
   return (height == (float)-MIN_KOD_INT ? -MIN_KOD_INT : FLOATTOKODINT(FINENESSROOTOKOD(height)));
}

Bool LineOfSightBSP(roomdata_node *r, int from_row, int from_col, int from_finerow, int from_finecol,
                    int to_row, int to_col, int to_finerow, int to_finecol)
{
   if (!r || r->file_info.TreeNodesCount == 0)
      return false;

   V3 s;
   s.X = FINENESSKODTOROO((float)from_col * 64.0f + (float)from_finecol);
   s.Y = FINENESSKODTOROO((float)from_row * 64.0f + (float)from_finerow);
	
   // get floor height with depth modifier
   V2 s2d = { s.X, s.Y };
   s.Z = BSPGetHeight(&r->file_info, &s2d, true, true) + OBJECTHEIGHTROO;
	
   V3 e;
   e.X = FINENESSKODTOROO((float)to_col * 64.0f + (float)to_finecol);
   e.Y = FINENESSKODTOROO((float)to_row * 64.0f + (float)to_finerow);

   // get floor height with depth modifier
   V2 e2d = { e.X, e.Y };
   e.Z = BSPGetHeight(&r->file_info, &e2d, true, true) + OBJECTHEIGHTROO;

   bool los = BSPLineOfSight(&r->file_info, &s, &e);

#if DEBUGLOS
   dprintf("LOS:%i S:(%1.2f/%1.2f/%1.2f) E:(%1.2f/%1.2f/%1.2f)", los, s.X, s.Y, s.Z, e.X, e.Y, e.Z);
#endif

   return los;
}
