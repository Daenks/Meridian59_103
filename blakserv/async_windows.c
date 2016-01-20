// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#include "blakserv.h"

#define MAX_MAINTENANCE_MASKS 15
char *maintenance_masks[MAX_MAINTENANCE_MASKS];
int num_maintenance_masks = 0;
char *maintenance_buffer = NULL;

Bool CheckMaintenanceMask(SOCKADDR_IN6 *addr,int len_addr);

void InitAsyncConnections(void)
{
    WSADATA WSAData;

    if (WSAStartup(MAKEWORD(1,1),&WSAData) != 0)
    {
        eprintf("InitAsyncConnections can't open WinSock!\n");
        return;
    }

    maintenance_buffer = (char *)malloc(strlen(ConfigStr(SOCKET_MAINTENANCE_MASK)) + 1);
    strcpy(maintenance_buffer,ConfigStr(SOCKET_MAINTENANCE_MASK));

    // now parse out each maintenance ip
    maintenance_masks[num_maintenance_masks] = strtok(maintenance_buffer,";");
    while (maintenance_masks[num_maintenance_masks] != NULL)
    {
        num_maintenance_masks++;
        if (num_maintenance_masks == MAX_MAINTENANCE_MASKS)
            break;
        maintenance_masks[num_maintenance_masks] = strtok(NULL,";");
    }

    /*
    {
        int i;
        for (i=0;i<num_maintenance_masks;i++)
        {
            dprintf("mask %i is [%s]\n",i,maintenance_masks[i]);
        }
    }
    */
}

void ExitAsyncConnections(void)
{
    if (WSACleanup() == SOCKET_ERROR)
        eprintf("ExitAsyncConnections can't close WinSock!\n");
}


void AsyncSocketAccept(SOCKET sock,int event,int error,int connection_type)
{
	SOCKET new_sock;
	SOCKADDR_IN6 acc_sin;    /* Accept socket address - internet style */
	int acc_sin_len;        /* Accept socket address length */
	SOCKADDR_IN6 peer_info;
	int peer_len;
	struct in6_addr peer_addr;
	connection_node conn;
	session_node *s;
	
	if (event != FD_ACCEPT)
	{
		eprintf("AsyncSocketAccept got non-accept %i\n",event);
		return;
	}
	
	if (error != 0)
	{
		eprintf("AsyncSocketAccept got error %i\n",error);
		return;
	}
	
	acc_sin_len = sizeof acc_sin; 

	new_sock = accept(sock,(struct sockaddr *) &acc_sin,&acc_sin_len);
	if (new_sock == SOCKET_ERROR) 
	{
		eprintf("AcceptSocketConnections accept failed, error %i\n",
			GetLastError());
		return;
	}
	
	peer_len = sizeof peer_info;
	if (getpeername(new_sock,(SOCKADDR *)&peer_info,&peer_len) < 0)
	{
		eprintf("AcceptSocketConnections getpeername failed error %i\n",
			GetLastError());
		return;
	}
	
	memcpy(&peer_addr, &peer_info.sin6_addr, sizeof(struct in6_addr));
	memcpy(&conn.addr, &peer_addr, sizeof(struct in6_addr));
	inet_ntop(AF_INET6, &peer_addr, conn.name, sizeof(conn.name));
	
	// Took out following line to prevent log files from becoming spammed with extra lines.
	// This line is extraneous because the outcome of the authentication is always posted to logs.
	// lprintf("Got connection from %s to be authenticated.\n", conn.name);
	
	if (connection_type == SOCKET_MAINTENANCE_PORT)
	{
		if (!CheckMaintenanceMask(&peer_info,peer_len))
		{
			lprintf("Blocked maintenance connection from %s.\n", conn.name);
			closesocket(new_sock);
			return;
		}
	}
	else
	{
		if (!CheckBlockList(&peer_addr))
		{
			lprintf("Blocked connection from %s.\n", conn.name);
			closesocket(new_sock);
			return;
		}
	}
	
	conn.type = CONN_SOCKET;
	conn.socket = new_sock;
	
	EnterServerLock();
	
	s = CreateSession(conn);
	if (s != NULL)
	{
		StartAsyncSession(s);
		
		switch (connection_type)
		{
		case SOCKET_PORT :
			InitSessionState(s,STATE_SYNCHED);   
			break;
		case SOCKET_MAINTENANCE_PORT :
			InitSessionState(s,STATE_MAINTENANCE);
			break;
		default :
			eprintf("AcceptSocketConnections got invalid connection type %i\n",connection_type);
		}
		
		/* need to do this AFTER s->conn is set in place, because the async
		call writes to that mem address */
		
		if (ConfigBool(SOCKET_DNS_LOOKUP))
		{
			// disabled due to IPv6 right now
			//s->conn.hLookup = StartAsyncNameLookup((char *)&peer_addr,s->conn.peer_data);
		}
		else
		{
			s->conn.hLookup = 0;
		}
	}
	
	LeaveServerLock();
}

void AsyncSocketSelect(SOCKET sock,int event,int error)
{
   session_node *s;

   EnterSessionLock();

   if (error != 0)
   {
      LeaveSessionLock();
      s = GetSessionBySocket(sock);
      if (s != NULL)
      {
      /* we can get events for sockets that have been closed by main thread
         (and hence get NULL here), so be aware! */

         /* eprintf("AsyncSocketSelect got error %i session %i\n",error,s->session_id); */

         HangupSession(s);
         return;
      }

      /* eprintf("AsyncSocketSelect got socket that matches no session %i\n",sock); */

      return;
   }

   switch (event)
   {
   case FD_CLOSE :
      AsyncSocketClose(sock);
      break;

   case FD_WRITE :
      AsyncSocketWrite(sock);
      break;

   case FD_READ :
      AsyncSocketRead(sock);
      break;

   default :
      eprintf("AsyncSocketSelect got unknown event %i\n",event);
      break;
   }

   LeaveSessionLock();
}


Bool CheckMaintenanceMask(SOCKADDR_IN6 *addr,int len_addr)
{
    IN6_ADDR mask;
    int i;
    BOOL skip;

    for (i=0;i<num_maintenance_masks;i++)
    {
        if (inet_pton(AF_INET6, maintenance_masks[i], &mask) != 1)
        {
            eprintf("CheckMaintenanceMask has invalid configured mask %s\n",
                      maintenance_masks[i]);
            continue;
        }

        /* for each byte of the mask, if it's non-zero, the client must match it */

        skip = 0;
        for (int k = 0; k < sizeof(mask.u.Byte); k++)
        {
            if (mask.u.Byte[k] != 0 && mask.u.Byte[k] != addr->sin6_addr.u.Byte[k])
            {
                // mismatch
                skip = 1;
                break;
            }
        }

        if (skip)
            continue;

        return True;
    }
    return False;
}

