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

typedef struct astarnode_struct
{
	int row, col;
	//F = MovementCost + HeuristicCost 
	int score;
	//G = (MovementCost), we need to take the G of its parent (the square where we came from) and to add 10 to it. 
	//Therefore, the G of each square will represent the total cost of the generated path from point A until the square).
	int movement_cost; 
	//H = To put it simply, we will use the “Manhattan distance method” (Also called “Manhattan length” or “city block distance”) 
	//that just counts the number of horizontal and vertical square remaining to reach point B without taking into account of any obstacles or differences of land.
	int heuristic_cost;
	//Next node on the path
	astarnode_struct *parent;
	//so we can make nodes into a linked list
	astarnode_struct *next;

} astar_node;

typedef struct astarpath_struct
{
	int startrow;
	int startcol;
	int endrow;
	int endcol;
	int roomid;
	roomdata_node *room;

	astar_node *grid;

	astar_node *closed; //linked list
	astar_node *open; //linked list

	int path_list_id;

} astar_path;

int CreatePath(int startrow, int startcol, int endrow, int endcol, int roomid);

#endif /*#ifndef _ASTAR_H */
