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
 
 Will create an AstarPath Kod object, calculate the path between the origin and target,
 and give that AstarPath Kod object a plPath list of the needed nodes to follow

 */

#include "blakserv.h"

//local prototypes
//node calculations
void CalculateMovementCost(astar_node *node, astar_path *path, bool diagonal);
void CalculateHeuristic(astar_node *node, astar_path *path);
void CalculateScore(astar_node *node, astar_path *path, bool diagonal);

//linked list accessors
void DebugPrintList(astar_node *head); //print the list
void InsertNodeToList(astar_node **head, astar_node *node); //keep the list insertion sorted as you create it
void RemoveNodeFromList(astar_node **head, astar_node *node); //remove this node from this list
bool IsNodeOnList(astar_node *head, astar_node *node);
void PushNodeToList(astar_node **head, astar_node *node);

//grid stuff
void CreateGrid(astar_path *path);
void DisplayGrid(astar_path *path);
void FreeGrid(astar_path *path);

//pathfinding
void ScanNode(astar_node *startnode, astar_path *path);

//Takes two objects, from and to (origin and target)
int CreatePath(int startrow, int startcol, int endrow, int endcol, int roomid)
{
	int rows, cols, idxstart, idxend;
	astar_path path;
	astar_node *startnode, *endnode;

	//gets the roomdata we need to use CanMoveInRoom()
	path.room = GetRoomDataByID(roomid);
	path.startrow = startrow;
	path.startcol = startcol;
	path.endrow = endrow;
	path.endcol = endcol;
	path.path_list_id = 0;
	path.open = NULL;
	path.closed = NULL;
	CreateGrid(&path); //creates our 2d grid of rows

	rows = path.room->file_info.rows;
	cols = path.room->file_info.cols;
	
	// rows and cols are 1-based here
	idxstart = (startrow - 1) * cols + (startcol - 1);
	idxend = (endrow - 1) * cols + (endcol - 1);

	startnode = &path.grid[idxstart];
	endnode = &path.grid[idxend];

	//Calculate its values
	CalculateScore(startnode, &path, false);
	//Add it to the open list
	InsertNodeToList(&path.open, startnode);

	//Calculate scores and parents for all our nodes
	while (!IsNodeOnList(path.closed, endnode))
	{
		//the lowest score node should always be the first item on the open list
		astar_node *lowestscorenode = path.open;
		if (lowestscorenode == NULL) //If the open list is empty we don't have a path.
		{
			return NIL;
		}
		//remove it from the open list
		RemoveNodeFromList(&path.open, lowestscorenode);
		//add it to the closed list
		InsertNodeToList(&path.closed, lowestscorenode);
		//scan it
		ScanNode(lowestscorenode, &path);
	}

	DisplayGrid(&path);

	//build our path by following the endnode's parents to the startnode
	while (endnode != NULL)
	{
		int coordinate_list;
		//Create a list [ [ row, col ], [ row, col ], etc...]
		val_type first_val,rest_val;
		first_val.v.tag = TAG_INT;
		first_val.v.data = endnode->col;
		rest_val.v.tag = TAG_NIL;
		coordinate_list = Cons(first_val,rest_val); // [ col ]

		first_val.v.tag = TAG_INT;
		first_val.v.data = endnode->row;
		rest_val.v.tag = TAG_LIST;
		rest_val.v.data = coordinate_list;
		coordinate_list = Cons(first_val,rest_val); // [ row, col ]

		first_val.v.tag = TAG_LIST;
		first_val.v.data = coordinate_list;

		if (path.path_list_id == 0)
		{	
			rest_val.v.tag = TAG_NIL; // if we dont yet have a superlist, create one
		}
		else
		{
			rest_val.v.tag = TAG_LIST;
			rest_val.v.data = path.path_list_id; // or append to it if we do
		}
		path.path_list_id = Cons(first_val,rest_val); // add [row, col] to the superlist of coordinate pairs

		endnode = endnode->parent;	
	}
	//so kod can interpret the list, tag it before returning it
	val_type return_val;
	return_val.v.tag = TAG_LIST;
	return_val.v.data = path.path_list_id;

	//Finally, clean up our grid, our path object
	FreeGrid(&path);

	return return_val.int_val;
}

void CreateGrid(astar_path *path)
{
	int rows = path->room->file_info.rows;
	int cols = path->room->file_info.cols;
	int size = rows * cols * sizeof(astar_node);
	int idx = 0;
	
	// allocate
	path->grid = (astar_node *)AllocateMemory(MALLOC_ID_ASTAR, size);

	// initialize
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0 ; col < cols; col++)
		{
			idx = row * cols + col;

			path->grid[idx].row = row + 1;
			path->grid[idx].col = col + 1;
			path->grid[idx].parent = NULL;
			path->grid[idx].score = 0;
			path->grid[idx].movement_cost = 0;
			path->grid[idx].heuristic_cost = 0;
		}
	}
}

void DisplayGrid(astar_path *path)
{
	int rows = path->room->file_info.rows;
	int cols = path->room->file_info.cols;
	int idx = 0;

	char *rowstring = (char *)AllocateMemory(MALLOC_ID_ASTAR,10000);
	dprintf("start: %d,%d end: %d,%d",path->startrow,path->startcol,path->endrow,path->endcol);
	for (int row = 0; row < rows; row++)
	{
		sprintf(rowstring,"Row %3d- ",row);
		for (int col = 0 ; col < cols; col++)
		{
			idx = row * cols + col;

			if (path->grid[idx].parent != NULL)
				sprintf(rowstring,"%s|F=%3d;P=%3d,%3d|",rowstring,path->grid[idx].score,path->grid[idx].parent->row,path->grid[idx].parent->col);
			else
				sprintf(rowstring,"%s|F=%3d;P=NULL   |",rowstring,path->grid[idx].score);
		}
		dprintf(rowstring);
	}
	FreeMemory(MALLOC_ID_ASTAR,rowstring,10000);
}

void FreeGrid(astar_path *path)
{
	int rows = path->room->file_info.rows;
	int cols = path->room->file_info.cols;
	int size = rows * cols * sizeof(astar_node);

	FreeMemory(MALLOC_ID_ASTAR, path->grid, size);			
}

void ScanNode(astar_node *startnode, astar_path *path)
{
	int idx, row, col;
	astar_node *currentnode;
	int rows = path->room->file_info.rows;
	int cols = path->room->file_info.cols;
	
	if (startnode->row > rows ||
		startnode->col > cols ||
		startnode->row < 1 ||
		startnode->col < 1)
	{
		dprintf("startnode Row or Col outside bounds in ScanNoe()");
		return;
	}

	for (int rowoffset = -1; rowoffset < 2; rowoffset++) //loop -1, 0, +1
	{
		for (int coloffset = -1; coloffset < 2; coloffset++) //loop -1, 0, +1
		{
			row = startnode->row + rowoffset;
			col = startnode->col + coloffset;

			//if the current node is outside the map boundary, skip it
			if (row > rows ||
				col > cols ||
				row < 1 ||
				col < 1)
			{
				continue;
			}
			
			// row and col are 1-based here
			idx = (row-1) * cols + (col-1);
			
			//grab our found node from the grid
			currentnode = &path->grid[idx];
			
			//if the current node is null, skip.
			if (currentnode == NULL)
				continue;
			//if currentnode == startnode, skip it
			if (currentnode == startnode)
				continue;
			//if the node is on the closed list, skip it
			if (IsNodeOnList(path->closed,currentnode))
				continue;
			//if we can move from startnode->row/col to currentnode->row/col
			if (CanMoveInRoom(path->room,startnode->row,startnode->col,currentnode->row,currentnode->col))
			{
				//if the node is not on the open list
				if (!IsNodeOnList(path->open,currentnode))
				{
					//set its parent
					currentnode->parent = startnode;
					//calculate its score
					CalculateScore(currentnode,path,(rowoffset !=0 && coloffset !=0)); // (rowoffset!=0 && coloffset!=0) == true when moving diagonal
					//add it to the open list
					InsertNodeToList(&path->open,currentnode);
				}
				//if the node is alread on the open list see if the new parent is better than the old parent
				else
				{
					if (startnode->parent != NULL)
					{
						//store the movement cost if we switched our parent to this one
						int newmovementcost;
						//if we are moving diagonally
						if (rowoffset !=0 && coloffset !=0) 
							newmovementcost = startnode->movement_cost + 14;
						else
							newmovementcost = startnode->movement_cost + 10;
						//if new parent movement cost is lower, set it.
						if (currentnode->parent !=NULL)
						{
							if (newmovementcost < currentnode->parent->movement_cost)
							{
								currentnode->parent = startnode;
								CalculateScore(currentnode,path,(rowoffset !=0 && coloffset !=0));
								//remove and re-add it to the open list, to keep the list sorted
								RemoveNodeFromList(&path->open,currentnode);
								InsertNodeToList(&path->open,currentnode);
							}
						}
						else
						{
							dprintf("currentnode had NULL parent");
							currentnode->parent = startnode;
							CalculateScore(currentnode,path,(rowoffset !=0 && coloffset !=0));
							//remove and re-add it to the open list, to keep the list sorted
							RemoveNodeFromList(&path->open,currentnode);
							InsertNodeToList(&path->open,currentnode);
						}
					}
					else
					{
						dprintf("Parent is null?");
					}
				}
			}
		}
	}
}

/***********************Start: Node Calculations********************/
void CalculateMovementCost(astar_node *node, astar_path *path, bool diagonal)
{
	if (node->parent != NULL) //if we have a parent
	{
		if (diagonal)
			node->movement_cost = node->parent->movement_cost + 14;
		else
			node->movement_cost = node->parent->movement_cost + 10;
	}
	else
	{
		node->movement_cost = 0;
	}
}

void CalculateHeuristic(astar_node *node, astar_path *path)
{
	node->heuristic_cost = (abs(path->endrow - node->row) * 10) + (abs(path->endcol - node->col) * 10);
}

void CalculateScore(astar_node *node, astar_path *path, bool diagonal)
{
	CalculateMovementCost(node,path,diagonal);
	CalculateHeuristic(node,path);
	node->score = node->movement_cost + node->heuristic_cost;
}

/*******************************************************************/
/***********************Start: List Functions***********************/
//Adds the node at the beginning of the list
void PushNodeToList(astar_node **head, astar_node *node)
{
	node->next=*head;
	*head = node;
}

//Insert into list where node->score fits in ascending order
void InsertNodeToList(astar_node **head, astar_node *node)
{
	for ( ; *head; head = &(*head)->next)
	{
		if ((*head)->score > node->score)
			break;
	}
	node->next = *head;
	*head = node;
}

//Remove a node from the list
void RemoveNodeFromList(astar_node **head, astar_node *node)
{
	astar_node *current = NULL,
			   *previous = NULL;
	for (current = *head; current != NULL; previous = current, current = current->next)
	{
		if (current == node)
		{
			if (previous == NULL)
			{
				*head = current->next;
			}
			else
			{
				previous->next = current->next;
			}
			return;
		}
	}
}

//Check if a node is on a list
bool IsNodeOnList(astar_node *head, astar_node *node)
{
	astar_node *current;
	for (current = head; current != NULL; current = current->next)
	{
		if (current == node)
			return true;
	}
	return false;
}

void DebugPrintList(astar_node *head)
{
	astar_node *current = head;
	while (current != NULL) 
	{
		dprintf("node: row %d,col %d,movecost %d,heuristic %d,score %d\n",
			current->row,current->col,current->movement_cost,current->heuristic_cost,current->score);
		current = current->next;
	}
}
/*******************************************************************/