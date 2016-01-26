// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* async.c
*

  This module contains functions to handle asynchronous socket events
  (i.e. new connections, reading/writing, and closing.
  
	Every function here is called from the interface thread!
	
*/

#include "blakserv.h"

static HANDLE name_lookup_handle;

/* local function prototypes */
void AcceptSocketConnections(int socket_port,int connection_type);
void AsyncEachSessionNameLookup(session_node *s);

void AsyncSocketStart(void)
{
	AcceptSocketConnections(ConfigInt(SOCKET_PORT),SOCKET_PORT);
	AcceptSocketConnections(ConfigInt(SOCKET_MAINTENANCE_PORT),SOCKET_MAINTENANCE_PORT);
}

/* connection_type is either SOCKET_PORT or SOCKET_MAINTENANCE_PORT, so we
keep track of what state to send clients into. */
void AcceptSocketConnections(int socket_port,int connection_type)
{
	SOCKET sock;
	SOCKADDR_IN6 sin;
	struct linger xlinger;
	int xxx;
	
	sock = socket(AF_INET6,SOCK_STREAM,0);
	if (sock == INVALID_SOCKET) 
	{
		eprintf("AcceptSocketConnections socket() failed WinSock code %i\n",
			GetLastError());
		closesocket(sock);
		return;
	}
	
	/* Make sure this is a IPv4/IPv6 dual stack enabled socket */
	
	xxx = 0;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&xxx, sizeof(xxx)) < 0)
	{
		eprintf("AcceptSocketConnections error setting sock opts: IPV6_V6ONLY\n");
		return;
	}

	/* Set a couple socket options for niceness */
	
	xlinger.l_onoff=0;
	if (setsockopt(sock,SOL_SOCKET,SO_LINGER,(char *)&xlinger,sizeof(xlinger)) < 0)
	{
		eprintf("AcceptSocketConnections error setting sock opts: SO_LINGER\n");
		return;
	}
	
	xxx=1;
	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&xxx,sizeof xxx) < 0)
	{
		eprintf("AcceptSocketConnections error setting sock opts: SO_REUSEADDR\n");
		return;
	}
	
	if (!ConfigBool(SOCKET_NAGLE))
	{
		/* turn off Nagle algorithm--improve latency? */
		xxx = true;
		if (setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char *)&xxx,sizeof xxx))
		{
			eprintf("AcceptSocketConnections error setting sock opts: TCP_NODELAY\n");
			return;
		}
	}
	
	memset(&sin, 0, sizeof(sin));
	sin.sin6_family = AF_INET6;
	sin.sin6_addr = in6addr_any;
	sin.sin6_flowinfo = 0;
	sin.sin6_scope_id = 0;
	sin.sin6_port = htons((short)socket_port);
	
	if (bind(sock,(struct sockaddr *) &sin,sizeof(sin)) == SOCKET_ERROR) 
	{
		eprintf("AcceptSocketConnections bind failed, WinSock error %i\n",
			GetLastError());
		closesocket(sock);
		return;
	}	  
	
	if (listen(sock,5) < 0) /* backlog of 5 connects by OS */
	{
		eprintf("AcceptSocketConnections listen failed, WinSock error %i\n",
			GetLastError());
		closesocket(sock);
		return;
	}
	
	StartAsyncSocketAccept(sock,connection_type);
	/* when we get a connection, it'll call AsyncSocketAccept */
}

void AsyncNameLookup(HANDLE hLookup,int error)
{
   if (error != 0)
   {
      /* eprintf("AsyncSocketNameLookup got error %i\n",error); */
      return;
   }
   
   name_lookup_handle = hLookup;
   
   EnterServerLock();
   ForEachSession(AsyncEachSessionNameLookup);
   LeaveServerLock();
   
}

void AsyncEachSessionNameLookup(session_node *s)
{
	if (s->conn.type != CONN_SOCKET)
		return;
	
	if (s->conn.hLookup == name_lookup_handle)
	{
		sprintf(s->conn.name,"%s",((struct hostent *)&(s->conn.peer_data))->h_name);
		InterfaceUpdateSession(s);
	}      
}

void AsyncSocketClose(SOCKET sock)
{
	session_node *s;
	
	s = GetSessionBySocket(sock);
	if (s == NULL)
		return;
	
	/* dprintf("async socket close %i\n",s->session_id); */
	HangupSession(s);
	
}

void AsyncSocketWrite(SOCKET sock)
{
   int bytes;  
   session_node *s;
   buffer_node *bn;

   s = GetSessionBySocket(sock);
   if (s == NULL)
      return;

   if (s->hangup)
      return;

   /* dprintf("got async write session %i\n",s->session_id); */
   if (!MutexAcquireWithTimeout(s->muxSend,10000))
   {
      eprintf("AsyncSocketWrite couldn't get session %i muxSend\n",s->session_id);
      return;
	}

   while (s->send_list != NULL)
   {
      bn = s->send_list;
      /* dprintf("async writing %i\n",bn->len_buf); */
      bytes = send(s->conn.socket,bn->buf,bn->len_buf,0);
      if (bytes == SOCKET_ERROR)
      {
         if (GetLastError() != WSAEWOULDBLOCK)
         {
            /* eprintf("AsyncSocketWrite got send error %i\n",GetLastError()); */
            if (!MutexRelease(s->muxSend))
               eprintf("File %s line %i release of non-owned mutex\n",__FILE__,__LINE__);

            HangupSession(s);
            return;
         }
			
         /* dprintf("got write event, but send would block\n"); */
         break;
      }
      else
      {
         if (bytes != bn->len_buf)
            dprintf("async write wrote %i/%i bytes\n",bytes,bn->len_buf);
			
         transmitted_bytes += bn->len_buf;
			
         s->send_list = bn->next;
         DeleteBuffer(bn);
      }
   }
   if (!MutexRelease(s->muxSend))
      eprintf("File %s line %i release of non-owned mutex\n",__FILE__,__LINE__);
}

void AsyncSocketRead(SOCKET sock)
{
   int bytes;
   session_node *s;
   buffer_node *bn;

   s = GetSessionBySocket(sock);
   if (s == NULL)
      return;

   if (s->hangup)
      return;

   if (!MutexAcquireWithTimeout(s->muxReceive,10000))
   {
      eprintf("AsyncSocketRead couldn't get session %i muxReceive",s->session_id);
      return;
   }

   if (s->receive_list == NULL)
   {
      s->receive_list = GetBuffer();
      /* dprintf("Read0x%08x\n",s->receive_list); */
   }
	
   // find the last buffer in the receive list
   bn = s->receive_list;
   while (bn->next != NULL)
      bn = bn->next;
	
   // if that buffer is filled to capacity already, get another and append it
   if (bn->len_buf >= bn->size_buf)
   {
      bn->next = GetBuffer();
      /* dprintf("ReadM0x%08x\n",bn->next); */
      bn = bn->next;
   }
	
   // read from the socket, up to the remaining capacity of this buffer
   bytes = recv(s->conn.socket,bn->buf + bn->len_buf,bn->size_buf - bn->len_buf,0);
   if (bytes == SOCKET_ERROR)
   {
      if (GetLastError() != WSAEWOULDBLOCK)
      {
         /* eprintf("AsyncSocketRead got read error %i\n",GetLastError()); */
         if (!MutexRelease(s->muxReceive))
            eprintf("File %s line %i release of non-owned mutex\n",__FILE__,__LINE__);

         HangupSession(s);
         return;
      }
      if (!MutexRelease(s->muxReceive))
         eprintf("File %s line %i release of non-owned mutex\n",__FILE__,__LINE__);
   }

   if (bytes == 0)
   {
      if (!MutexRelease(s->muxReceive))
         eprintf("File %s line %i release of non-owned mutex\n",__FILE__,__LINE__);

      HangupSession(s);

      return;
   } 

   if (bytes < 0 || bytes > bn->size_buf - bn->len_buf)
   {
      eprintf("AsyncSocketRead got %i bytes from recv() when asked to stop at %i\n",bytes,bn->size_buf - bn->len_buf);
      FlushDefaultChannels();
      bytes = 0;
   }

   bn->len_buf += bytes;
	
   if (!MutexRelease(s->muxReceive))
      eprintf("File %s line %i release of non-owned mutex\n",__FILE__,__LINE__);  
	
	SignalSession(s->session_id);
}
