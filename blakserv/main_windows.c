// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* main.c
*

  Blakserv is the server program for Blakston.  This is a windows application,
  so we have a WinMain.  There is a dialog box interface in interfac.c.
  
	This module starts all of our "subsystems" and calls the timer loop,
	which executes until we terminate (either by the window interface or by
	a system administrator logging in to administrator mode.
	
*/

#include "blakserv.h"

void MainServer()
{
	InitInterfaceLocks(); 
	
	InitInterface(); /* starts a thread with the window */

	InitMemory(); /* memory needs channels in general, but need to start before config,
	so just be careful. */
	
	InitConfig();
	LoadConfig();		/* must be nearly first since channels use it */

	InitDebug();
	
	InitChannelBuffer();
	
	OpenDefaultChannels();

	if (ConfigBool(MYSQL_ENABLED))
	{
		lprintf("Starting MySQL writer");
		MySQLInit(ConfigStr(MYSQL_HOST), ConfigStr(MYSQL_USERNAME), ConfigStr(MYSQL_PASSWORD), ConfigStr(MYSQL_DB));
	}

	lprintf("Starting %s\n",BlakServLongVersionString());

	InitClass();
	InitMessage();
	InitObject();
	InitList();
	InitTimer();
	InitSession();
	InitResource();
	InitRooms();
	InitString();
	InitUser();
	InitAccount();
	InitNameID();
	InitDLlist();   
	InitSysTimer();
	InitMotd();
	InitLoadBof();
	InitTime();
	InitGameLock();
	InitBkodInterpret();
	InitBufferPool();
	InitTables();
	AddBuiltInDLlist();
	LoadMotd();
	LoadBof();
	LoadRsc();
	LoadKodbase();
	LoadAdminConstants();
	PauseTimers();
	
	if (LoadAll() == True)
	{
	/* this loaded_game_msg tells it to disconnect all blakod info about sessions,
		* that were logged on when we saved */
		
		SendTopLevelBlakodMessage(GetSystemObjectID(),LOADED_GAME_MSG,0,NULL);
		DoneLoadAccounts();
	}
	
	/* these must be after LoadAll and ClearList */
	InitCommCli(); 
	InitParseClient(); 
	InitProfiling();
	InitAsyncConnections();
	UpdateSecurityRedbook();
	UnpauseTimers();

	ServiceTimers(); /* returns if server termiated */

	MainExitServer();
}

void MainExitServer()
{
	lprintf("ExitServer terminating server\n");
	
	ExitAsyncConnections();
	
	CloseAllSessions(); /* gotta do this before anything, cause it uses kod, accounts */
	
	CloseDefaultChannels();
	
	ResetLoadMotd();
	ResetLoadBof();
	
	ResetTables();
	ResetBufferPool();
	ResetSysTimer();
	ResetDLlist();
	ResetNameID();
	ResetAccount();
	ResetUser();
	ResetString();
	// ExitRooms calls ResetRooms in addition to clearing the array memory.
	ExitRooms();
	ResetResource();
	ResetTimer();
	ResetList();
	ResetObject();
	ResetMessage();
	ResetClass();

	if (ConfigBool(MYSQL_ENABLED))
		MySQLEnd();
	
	ResetConfig();
	
	DeleteAllBlocks();
}
