// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* astar_classes.h: Classes used by astar, mainly for the queue items.
*
*/

#ifndef _ASTAR_CLASSES__H
#define _ASTAR_CLASSES__H

#include <list>
#include <map>

/*********************************************************************************************************************/

typedef struct astar_node astar_node;
typedef struct astar_path_node astar_path_node;
typedef struct astar_room astar_room;

/*********************************************************************************************************************/

class astar_path : public ::std::list<astar_path_node*> { };

/*********************************************************************************************************************/

class astar_rooms : public ::std::map<int, astar_room*> { };

/*********************************************************************************************************************/
/*                                                 ENUMS                                                             */
/*********************************************************************************************************************/

typedef enum astar_command_type
{
   AStarLoadRoom = 1,
   AStarUnloadRoom = 2,
   AStarBlockerAdd = 3,
   AStarBlockerRemove = 4,
   AStarBlockerMove = 5,
   AStarBlockerClear = 6,
   AStarMoveSector = 7,
   AStarChangeTexture = 8
} astar_command_type;

typedef enum astar_query_type
{
   AStarPathQuery = 1
} astar_query_type;

typedef enum astar_response_type
{
   AStarPathResponse = 1
} astar_response_type;

/*********************************************************************************************************************/
/*                                               COMMANDS                                                            */
/*********************************************************************************************************************/

class astar_command
{
public:
   virtual astar_command_type commandType() = 0;
   virtual astar_command* clone() = 0;
};

/*********************************************************************************************************************/

class astar_command_loadroom : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarLoadRoom; }

   char* file;
   int id;

   astar_command_loadroom(char* file, int id)
   {
      astar_command_loadroom::file = strdup(file);
      astar_command_loadroom::id = id;
   }

   ~astar_command_loadroom()
   {
      free(file);
   }

   virtual astar_command* clone() override
   {
      return new astar_command_loadroom(file, id);
   }
};

/*********************************************************************************************************************/

class astar_command_unloadroom : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarUnloadRoom; }

   int id;

   astar_command_unloadroom(int id)
   {
      astar_command_unloadroom::id = id;
   }

   virtual astar_command* clone() override
   {
      return new astar_command_unloadroom(id);
   }
};

/*********************************************************************************************************************/

class astar_command_blockeradd : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarBlockerAdd; }

   int roomid;
   int objectid;
   V2 p;

   astar_command_blockeradd(int roomid, int objectid, V2 p)
   {
      astar_command_blockeradd::roomid = roomid;
      astar_command_blockeradd::objectid = objectid;
      astar_command_blockeradd::p = p;
   }

   virtual astar_command* clone() override
   {
      return new astar_command_blockeradd(roomid, objectid, p);
   }
};

/*********************************************************************************************************************/

class astar_command_blockerremove : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarBlockerRemove; }

   int roomid;
   int objectid;

   astar_command_blockerremove(int roomid, int objectid)
   {
      astar_command_blockerremove::roomid = roomid;
      astar_command_blockerremove::objectid = objectid;
   }

   virtual astar_command* clone() override
   {
      return new astar_command_blockerremove(roomid, objectid);
   }
};

/*********************************************************************************************************************/

class astar_command_blockermove : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarBlockerMove; }

   int roomid;
   int objectid;
   V2 p;

   astar_command_blockermove(int roomid, int objectid, V2 p)
   {
      astar_command_blockermove::roomid = roomid;
      astar_command_blockermove::objectid = objectid;
      astar_command_blockermove::p = p;
   }

   virtual astar_command* clone() override
   {
      return new astar_command_blockermove(roomid, objectid, p);
   }
};

/*********************************************************************************************************************/

class astar_command_blockerclear : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarBlockerClear; }

   int roomid;

   astar_command_blockerclear(int roomid)
   {
      astar_command_blockerclear::roomid = roomid;
   }

   virtual astar_command* clone() override
   {
      return new astar_command_blockerclear(roomid);
   }
};

/*********************************************************************************************************************/

class astar_command_movesector : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarMoveSector; }

   int roomid;
   unsigned int serverid;
   bool floor;
   float height;
   float speed;

   astar_command_movesector(int roomid, unsigned int serverid, bool floor, float height, float speed)
   {
      astar_command_movesector::roomid = roomid;
      astar_command_movesector::serverid = serverid;
      astar_command_movesector::floor = floor;
      astar_command_movesector::height = height;
      astar_command_movesector::speed = speed;
   }

   virtual astar_command* clone() override
   {
      return new astar_command_movesector(roomid, serverid, floor, height, speed);
   }
};

/*********************************************************************************************************************/

class astar_command_changetexture : public astar_command
{
public:
   virtual astar_command_type commandType() override { return astar_command_type::AStarChangeTexture; }

   int roomid;
   unsigned int serverid;
   unsigned short newtexture;
   unsigned int flags;

   astar_command_changetexture(int roomid, unsigned int serverid, unsigned short newtexture, unsigned int flags)
   {
      astar_command_changetexture::roomid = roomid;
      astar_command_changetexture::serverid = serverid;
      astar_command_changetexture::newtexture = newtexture;
      astar_command_changetexture::flags = flags;
   }

   virtual astar_command* clone() override
   {
      return new astar_command_changetexture(roomid, serverid, newtexture, flags);
   }
};

/*********************************************************************************************************************/

class astar_command_queue : public threadsafe_queue<astar_command*>
{
public:
   astar_command_queue(int size) : threadsafe_queue<astar_command*>(size)
   {
   }
};

/*********************************************************************************************************************/
/*                                                QUERIES                                                            */
/*********************************************************************************************************************/

class astar_query
{
public:
   virtual astar_query_type queryType() = 0;
};


/*********************************************************************************************************************/

class astar_pathquery : public astar_query
{
public:
   virtual astar_query_type queryType() override { return astar_query_type::AStarPathQuery; }

   int roomid;
   int objectid;
   V2 s;
   V2 e;
   unsigned int flags;

   astar_pathquery(int roomid, int objectid, V2 s, V2 e, unsigned int flags)
   {
      astar_pathquery::roomid = roomid;
      astar_pathquery::objectid = objectid;
      astar_pathquery::s = s;
      astar_pathquery::e = e;
      astar_pathquery::flags = flags;
   }
};

/*********************************************************************************************************************/

class astar_query_queue : public threadsafe_queue<astar_query*>
{
public:
   astar_query_queue(int size) : threadsafe_queue<astar_query*>(size)
   {
   }
};

/*********************************************************************************************************************/
/*                                                RESPONSES                                                          */
/*********************************************************************************************************************/

class astar_response
{
public:
   virtual astar_response_type responseType() = 0;
};

/*********************************************************************************************************************/

class astar_pathresponse : public astar_response
{
public:
   virtual astar_response_type responseType() override { return astar_response_type::AStarPathResponse; }

   int roomid;
   int objectid;
   V2 s;
   V2 e;
   unsigned int flags;
   astar_path path;

   astar_pathresponse(int roomid, int objectid, V2 s, V2 e, unsigned int flags)
   {
      astar_pathresponse::roomid = roomid;
      astar_pathresponse::objectid = objectid;
      astar_pathresponse::s = s;
      astar_pathresponse::e = e;
      astar_pathresponse::flags = flags;
   }
};

/*********************************************************************************************************************/

class astar_response_queue : public threadsafe_queue<astar_response*>
{
public:
	astar_response_queue(int size) : threadsafe_queue<astar_response*>(size)
	{
	}
};

#endif /*#ifndef _ASTAR_CLASSES__H */