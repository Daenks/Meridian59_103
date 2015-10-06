// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* astar.h:
*/

#ifndef _ASTAR_H
#define _ASTAR_H

#define CLOSEENOUGHDIST   3
#define ASTARENABLED      1

typedef struct room_type room_type;
typedef struct astar_node_data astar_node_data;
typedef struct astar_node astar_node;

typedef struct astar_node_data
{
   float       cost;
   float       heuristic;
   astar_node* parent;
   astar_node* nextopen;
   bool        isInClosedList;
} astar_node_data;

typedef struct astar_node
{
   int              Row;
   int              Col;
   V2               Location;
   astar_node_data* Data;
} astar_node;

typedef struct astar
{
   astar_node_data* NodesData;
   int              NodesDataSize;
   astar_node**     Grid;
   astar_node*      Open;
   astar_node*      EndNode;
   astar_node*      LastNode;
   int              ObjectID;
} astar;

void AStarGenerateGrid(room_type* Room);
void AStarFreeGrid(room_type* Room);
bool AStarGetStepTowards(room_type* Room, V2* S, V2* E, V2* P, unsigned int* Flags, int ObjectID);

#endif /*#ifndef _ASTAR_H */