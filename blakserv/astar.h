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

#include <map>
#include <thread>
#include <chrono>
#include <atomic>

/**************************************************************************************************************/
/*                                  GENERAL                                                                   */
/**************************************************************************************************************/

#define CLOSEENOUGHDIST     3   // allowed offset in squares to consider a square to be the end
#define DESTBLOCKIGNORE     3   // area around the end which never gets marked blocked by blockers

/**************************************************************************************************************/
/*                                  GRID                                                                      */
/**************************************************************************************************************/
#define COST               1.0f                        // cost of a straight-edge
#define COST_DIAG          ((float)M_SQRT2)            // cost of a diagonal-edge
#define MAXGRIDROWS        1024                        // max. supported highres rows (1 highres = 16 fine)
#define MAXGRIDCOLS        1024                        // max. supported highres cols (1 highres = 16 fine)
#define NODESDATASQUARES   MAXGRIDROWS * MAXGRIDCOLS   // max. supported highres-squares (rows*cols)

/**************************************************************************************************************/
/*                                  PATH CACHE                                                                */
/**************************************************************************************************************/
#define PATHCACHESIZE               16       // how many paths are cached per room
#define PATHCACHEENDTOLERANCE       768.0f   // max. distance between end and a cached end to be a match
#define PATHCACHESTARTTOLERANCE     16.0f    // max. distance between start and a cached start to be a match

/**************************************************************************************************************/
/*                                  NOPATH CACHE                                                              */
/**************************************************************************************************************/
#define NOPATHCACHESIZE             16       // how many unreachable start-end results are cached per room
#define NOPATHCACHEVALIDDURATIONINS 3        // how long a no-path cache entry is valid (in seconds)
#define NOPATHCACHEENDTOLERANCE     256.0f   // max. distance between end and a cached end to be a match
#define NOPATHCACHESTARTTOLERANCE   256.0f   // max. distance between start and a cached start to be a match

/**************************************************************************************************************/
/*                                  THREADING                                                                 */
/**************************************************************************************************************/
#define THREADSCOUNT        2      // how many astar workers (threads) are used
#define COMMANDQUEUESIZE    3000   // max. elements in command-queues (must be big, e.g. to sync room changes)
#define QUERYQUEUESIZE      100    // max. elements in query-queue
#define RESPONSEQUEUESIZE   100    // max. elements in response-queue
#define WORKERSLEEPTIMEMS   10     // sleep time for workers if they should sleep

/**************************************************************************************************************/
/*                                  EDGE CACHE FLAGS                                                          */
/**************************************************************************************************************/

#define EDGECACHE_KNOWS_N  0x0001   // set if north-edge is cached
#define EDGECACHE_KNOWS_NE 0x0002   // set if north-east-edge is cached
#define EDGECACHE_KNOWS_E  0x0004   // set if east-edge is cached
#define EDGECACHE_KNOWS_SE 0x0008   // set if south-east-edge is cached
#define EDGECACHE_KNOWS_S  0x0010   // set if south-edge is cached
#define EDGECACHE_KNOWS_SW 0x0020   // set if south-west-edge is cached
#define EDGECACHE_KNOWS_W  0x0040   // set if west-edge is cached
#define EDGECACHE_KNOWS_NW 0x0080   // set if north-west-edge is cached
#define EDGECACHE_CAN_N    0x0101   // set if north-edge is cached and walkable
#define EDGECACHE_CAN_NE   0x0202   // set if north-east-edge is cached and walkable
#define EDGECACHE_CAN_E    0x0404   // set if east-edge is cached and walkable
#define EDGECACHE_CAN_SE   0x0808   // set if south-east-edge is cached and walkable
#define EDGECACHE_CAN_S    0x1010   // set if south-edge is cached and walkable
#define EDGECACHE_CAN_SW   0x2020   // set if south-west-edge is cached and walkable
#define EDGECACHE_CAN_W    0x4040   // set if west-edge is cached and walkable
#define EDGECACHE_CAN_NW   0x8080   // set if north-west-edge is cached and walkable

/**************************************************************************************************************/
/*                                 FORWARD DECLARATIONS                                                       */
/**************************************************************************************************************/

typedef struct room_type room_type;
typedef struct BspLeaf BspLeaf;
typedef struct astar_node astar_node;

/**************************************************************************************************************/
/*                                        STRUCTS                                                             */
/**************************************************************************************************************/

typedef struct astar_node_data
{
   float       cost;            // cost of that node
   float       heuristic;       // heuristic cost of that node
   float       combined;        // sum of cost and heuristic
   astar_node* parent;          // parent (one of the neighbours) which provides the shortest path to the node
   int         heapindex;       // index of this node in the heap
   bool        isInClosedList;  // true for any evaluated node
   bool        isBlocked;       // true for any node blocked by a blocker
   astar_node* heapslot;        // provides the n-th memory slot of the heap (unrelated other values/the node) 
} astar_node_data;

/**************************************************************************************************************/

typedef struct astar_node_meta
{
   int  Row;          // row of the node (in highres)
   int  Col;          // column of the node (in highres)
   V2   Location;     // location (in ROO fineness)
} astar_node_meta;

/**************************************************************************************************************/

typedef struct astar_node
{
   astar_node_meta* Meta;     // the common meta data (equal for all nodes of any grid)
   astar_node_data* Data;     // the data-slot linked with this node
   BspLeaf*         Leaf;     // a possible BSP leaf at the node's location
   unsigned short*  Edges;    // pointer to the edge-cache of this node
} astar_node;

/**************************************************************************************************************/

typedef struct astar_path_node
{
   int Row;         // row of the node (in highres)
   int Col;         // col of the node (in highres)
   V2  Location;    // location (in ROO fineness)
} astar_path_node;

/**************************************************************************************************************/

typedef struct astar_room
{
   room_type*       Room;              // the basic bsp room-data
   astar_node**     Grid;              // the astar-grid of this room
   unsigned short*  EdgesCache;        // the edges-cache of this room
   int              EdgesCacheSize;    // size of the edges-cache in bytes
} astar_room;

/**************************************************************************************************************/

typedef struct astar_nopath
{
   V2 S;
   V2 E;
   ::std::chrono::system_clock::time_point Tick;
} astar_nopath;

/**************************************************************************************************************/

typedef struct astar
{
   ::std::thread*              Thread;          // the thread using this struct instance
   ::std::atomic<bool>         IsRunning;       // true if the worker is working
   ::std::atomic<bool>         IsPaused;        // true if the worker is paused
   ::std::atomic<bool>         DoPause;         // flip to true to pause this worker (may take some time)
   astar_node_data*            NodesData;       // the nodes-data memory used by this worker
   int                         NodesDataSize;   // sie of nodesdata in bytes
   astar_node_meta**           Grid;            // the meta-grid used by this worker
   astar_node*                 StartNode;       // start-node of a path-calculation
   astar_node*                 EndNode;         // end-node of a path-calculation
   astar_node*                 LastNode;        // holds the last-node on a path (null if unreachable)
   int                         ObjectID;        // object id we're calculating a path for
   int                         HeapSize;        // current size of the open-heap
   astar_room*                 Room;            // the room we calculate a path in
   astar_rooms*                Rooms;           // all room intances this worker is using
   astar_command_queue*        Commands;        // holds pending commands supposed to be processed by worker
   ::std::atomic<unsigned int> Memory;
} astar;

/**************************************************************************************************************/
/*                                 WORKER AND QUEUE INSTANCES                                                 */
/**************************************************************************************************************/

extern astar                AStar[THREADSCOUNT];    // the astar-workers
extern astar_query_queue    AStarQueries;           // the query-queue to send tasks to the workers
extern astar_response_queue AStarResponses;         // the response-queue to retrieve answers from workers

/**************************************************************************************************************/
/*                                      C FUNCTIONS                                                           */
/**************************************************************************************************************/

void AStarInit();
void AStarShutdown();
bool AStarDeliverNextResponse();
void AStarEnqueueCommand(astar_command* Command);
void AStarInitGC();
void AStarFinishGC();
void AStarPerformGC();

__inline void* AStarAlloc(astar* Astar, unsigned int Size)
{
   void* mem = malloc(Size);
   Astar->Memory += Size;
   return mem;
}

__inline void AStarFree(astar* Astar, void* Mem, unsigned int Size)
{
   free(Mem);
   Astar->Memory -= Size;
}

#endif /*#ifndef _ASTAR_H */