// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#include "blakserv.h"

int sessions_logged_on;
int console_session_id;
pthread_t interface_thread;

#define ADMIN_RESPONSE_SIZE (256 * 1024)

char admin_response_buf[ADMIN_RESPONSE_SIZE+1];
int len_admin_response_buf;

void InterfaceAddAdminBuffer(char *buf,int len_buf);

void InitInterface(void)
{
   int err;

   err = pthread_create(&interface_thread, NULL, &InterfaceMainLoop, NULL);

   if (err != 0)
   {
      eprintf("Unable to start interface! %s",strerror(err));
   }
   else
   {
      dprintf("interface thread started");
   }
}

void* InterfaceMainLoop(void* arg)
{
   char *line = (char*) malloc(200);
   size_t size;
   //char buf[200];

   while (strcmp(line,"quit") != 0)
   {
      printf("blakadm> ");
      if (getline(&line, &size, stdin) != -1)
      {
      
         EnterServerLock();

         // TODO: in windows this uses a set char array of size 200, no bounds checking
         // is being done here yet
         //TryAdminCommand(console_session_id,buf);
         TryAdminCommand(console_session_id,line);

         LeaveServerLock();

      }
   }

   free(line);
   MessagePost(main_thread_id,WM_QUIT,0,0);
}

void StartupPrintf(const char *fmt,...)
{
   char s[200];
   va_list marker;

   va_start(marker,fmt);
   vsprintf(s,fmt,marker);

   if (strlen(s) > 0)
   {
      if (s[strlen(s)-1] == '\n') /* ignore \n char at the end of line */
      s[strlen(s)-1] = 0;
   }

   va_end(marker);

   // TODO: Actually print something to stdout
}

// XXX: identical to windows version
void StartupComplete(void)
{
   char str[200];
   connection_node conn;
   session_node *s;

   len_admin_response_buf = 0;

   conn.type = CONN_CONSOLE;
   s = CreateSession(conn);
   if (s == NULL)
   FatalError("Interface can't make session for console");
   s->account = GetConsoleAccount();
   InitSessionState(s,STATE_ADMIN);
   console_session_id = s->session_id;
}

// XXX: identical to windows version
int GetUsedSessions(void)
{
   return sessions_logged_on;
}

void InterfaceUpdate(void)
{
    // TODO: stub
}

void InterfaceLogon(session_node *s)
{
    // TODO: stub
}

void InterfaceLogoff(session_node *s)
{
    // TODO: stub
}

void InterfaceUpdateSession(session_node *s)
{
    // TODO: stub
}

void InterfaceUpdateChannel(void)
{
    // TODO: stub
}

// called in main thread
// XXX: identical to windows version
void InterfaceSendBufferList(buffer_node *blist)
{
   buffer_node *bn;

   bn = blist;
   while (bn != NULL)
   {
      InterfaceAddAdminBuffer(bn->buf,bn->len_buf);
      bn = bn->next;
   }

   DeleteBufferList(blist);
}

// called in main thread
// XXX: identical to windows version
void InterfaceSendBytes(char *buf,int len_buf)
{
   InterfaceAddAdminBuffer(buf,len_buf);
}

// in main thread called by InterfaceSendBytes
void InterfaceAddAdminBuffer(char *buf,int len_buf)
{
   if (len_buf > ADMIN_RESPONSE_SIZE)
      len_buf = ADMIN_RESPONSE_SIZE;

   if (len_admin_response_buf + len_buf > ADMIN_RESPONSE_SIZE)
   {
      len_admin_response_buf = 0;
   }
   memcpy(admin_response_buf+len_admin_response_buf,buf,len_buf);
   len_admin_response_buf += len_buf;
   admin_response_buf[len_admin_response_buf] = 0;

   // supposedly writing to stdout is thread safe although the threads
   // are going to clobber each others output.  there is probably a better
   // way but for now output is sent directly to stdout
   printf(admin_response_buf);
}

void FatalErrorShow(const char *filename,int line,const char *str)
{
    // TODO: stub
}

