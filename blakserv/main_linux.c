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

int MainServer(int argc, char** argv)
{
   char c;
   bool iFaceFlag = false;

   InitMsgQueue();

   InitInterfaceLocks(); 

   while ((c = getopt(argc,argv,"hi")) != -1)
   {
      switch (c)
      {
      case 'i':
         iFaceFlag = true;
         break;

      case 'h':
         // TODO: Display Help
         break;

      case '?':
         if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                    optopt);
         return false;

      default:
         return false;;
      }
   }

   // launch interface or daamonize if desired
   if (iFaceFlag)
   {
      InitInterface();
   }
   else
   {
      Daemonize();
   }
	
   InitMemory(); /* memory needs channels in general, but need to start before config,
	so just be careful. */
	
   InitConfig();
   LoadConfig();		/* must be nearly first since channels use it */

   InitDebug();
	
   InitChannelBuffer();
	
   OpenDefaultChannels();

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

void Daemonize()
{
   pid_t pid, sid;
   int fd;

   pid = fork();

   if (pid < 0)
   {
      // failed to fork, exit with error
      eprintf("Error white trying to fork to the background, failed to fork!");
      exit(-1);
   }

   sid = setsid();

   if (sid < 0)
   {
      // failed to set sid, exit with error
      eprintf("Error white trying to fork to the background, failed to set sid!");
      exit(-1);
   }

   fd = open("/dev/null",O_RDWR, 0);  
  
   if (fd != -1)  
   {  
      dup2(fd, STDIN_FILENO);  
      dup2(fd, STDOUT_FILENO);  
      dup2(fd, STDERR_FILENO);  
  
      if (fd > 2)  
      {  
         close(fd);  
      }  
   }  
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

   ResetConfig();
	
   DeleteAllBlocks();
}
