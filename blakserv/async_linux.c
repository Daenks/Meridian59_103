// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#include "blakserv.h"

#define MAX_EPOLL_EVENTS 64
#define EPOLL_QUEUE_LEN 64

pthread_t network_thread;
pthread_t maintenance_thread;

#define MAX_MAINTENANCE_MASKS 15
char *maintenance_masks[MAX_MAINTENANCE_MASKS];
int num_maintenance_masks = 0;
char *maintenance_buffer = NULL;

int MakeNonBlockingSocket(int s);
void* NetworkWorker (void* arg);

typedef struct {
   int socket;
   int connection_type;
} nWrkArgs;

Bool CheckMaintenanceMask(SOCKADDR_IN6 *addr,int len_addr);

void InitAsyncConnections(void)
{

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
    // TODO: EMPTY function!
}

void StartAsyncSocketAccept(SOCKET sock,int connection_type)
{
   
   int err;

   nWrkArgs* args = (nWrkArgs*) malloc(sizeof(nWrkArgs));

   args->socket = sock;
   args->connection_type = connection_type;

//   err = pthread_create(&network_thread, NULL, &NetworkWorker, &args);

   if (connection_type == SOCKET_PORT)
   {
      err = pthread_create(&network_thread, NULL, &NetworkWorker, args);
   }
   else if (connection_type == SOCKET_MAINTENANCE_PORT)
   {
      err = pthread_create(&maintenance_thread, NULL, &NetworkWorker, args);
   }
   else
   {
      eprintf("Unable to start network worker thread! (Unknown port type)");
   }

   if (err != 0)
   {
      eprintf("Unable to start network worker thread! %s",strerror(err));
   }
   else
   {
      dprintf("network worker thread started for connection type %d on fd %d",connection_type,sock);
   }
   
}

HANDLE StartAsyncNameLookup(char *peer_addr,char *buf)
{
   // TODO: stub
   return 0;
}

void StartAsyncSession(session_node *s)
{
   // TODO: stub
}

void* NetworkWorker (void* _args)
{
   nWrkArgs* args = (nWrkArgs*) _args;

   int epoll_fd;
   int incoming_fd;

   int num_fds;
   int ret_val;

   struct epoll_event evt;
   struct epoll_event* events;

   epoll_fd = epoll_create(EPOLL_QUEUE_LEN);

   evt.events = EPOLLIN | EPOLLET;
   evt.data.fd = args->socket;

   events = (epoll_event*) calloc(MAX_EPOLL_EVENTS, sizeof evt);

   if (MakeNonBlockingSocket(args->socket) == -1)
   {
      eprintf("error in network worker thread! (make nonblock socket)");   
   } 

   // TODO: Nothing is done with ret_val
   ret_val = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, args->socket, &evt);

   // main loop for the network worker thread
   while (true)
   {
      num_fds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

      if (num_fds < 0)
      {
         eprintf("error in network worker thread! (epoll_wait)!");
      }

      for(int i = 0; i < num_fds; i++)
      {
         if ((events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN)))
         {
            // error on socket
            eprintf ("error in network worker thread! (socket error)");
            close (events[i].data.fd);

            continue;
         }
         else if(events[i].data.fd == args->socket)
         {
            // incoming connection(s)
            // keep accepting until we get EAGAIN or EWOULDBLOCK or until
            // some other socket error occurs
            while (true)
            {
               incoming_fd = AsyncSocketAccept(events[i].data.fd, FD_ACCEPT, 0, args->connection_type);
               if (incoming_fd != SOCKET_ERROR)
               {
                   if (MakeNonBlockingSocket(incoming_fd) == -1)
                   {
                       eprintf("error in network worker thread! (make nonblock socket)");
                   }
                   else
                   {
                       dprintf("accepted async socket on fd %d",incoming_fd);
                   }

                   // add this socket to the group of sockets to monitor
                   evt.data.fd = incoming_fd;
                   evt.events = EPOLLIN | EPOLLET;

                   // TODO: Nothing is done with ret_val
                   ret_val = epoll_ctl(epoll_fd,EPOLL_CTL_ADD,incoming_fd,&evt);
               }
               else
               {
                  // either all connections have been accepted or an error
                  // occured (which would have been logged already)
                  break;
               }
            }
         }
         else
         {
            // ready to read
            EnterSessionLock();
            AsyncSocketRead(events[i].data.fd);
            LeaveSessionLock();
         }
      }
   }

   free(args);
}

int AsyncSocketAccept(SOCKET sock,int event,int error,int connection_type)
{
    SOCKET new_sock;
    SOCKADDR_IN6 acc_sin;    /* Accept socket address - internet style */
    socklen_t acc_sin_len;        /* Accept socket address length */
    SOCKADDR_IN6 peer_info;
    socklen_t peer_len;
    struct in6_addr peer_addr;
    connection_node conn;
    session_node *s;

    if (event != FD_ACCEPT)
    {
        eprintf("AsyncSocketAccept got non-accept %i\n",event);
        return 0;
    }

    if (error != 0)
    {
        eprintf("AsyncSocketAccept got error %i\n",error);
        return 0;
    }

    acc_sin_len = sizeof acc_sin;

    new_sock = accept(sock,(struct sockaddr *) &acc_sin,&acc_sin_len);

    if (new_sock == SOCKET_ERROR)
    {
        // don't report EAGAIN or EWOULDBLOCK, we will get those regularly
        // as we process potential multiple simultaneous connections
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            eprintf("AcceptSocketConnections accept failed, error %i\n",
                GetLastError());
        }
        return SOCKET_ERROR;
    }

    peer_len = sizeof peer_info;

    if (getpeername(new_sock,(SOCKADDR *)&peer_info,&peer_len) < 0)
    {
        eprintf("AcceptSocketConnections getpeername failed error %i\n",
            GetLastError());
        return 0;
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
            return 0;
        }
    }
    else
    {
        if (!CheckBlockList(&peer_addr))
        {
            lprintf("Blocked connection from %s.\n", conn.name);
            closesocket(new_sock);
            return 0;
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

    // return the new socket to the network worker so it can watch it for incoming data
    return new_sock;
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

        for (int k = 0; k < sizeof(mask.s6_addr); k++)
        {
            if (mask.s6_addr[k] != 0 && mask.s6_addr[k] != addr->sin6_addr.s6_addr[k])
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

int MakeNonBlockingSocket(int s)
{
   int flags;
   int ret_val;

   // dont clobber stdin, stdout or stderr
   if (s < 3)
   {
      eprintf("refusing to clobber fd %d",s);
      printf("refusing to clobber fd %d\n",s);
      return -1;
   }

   flags = fcntl(s, F_GETFL, 0);
   flags |= O_NONBLOCK;
   ret_val = fcntl(s, F_SETFL, flags);

   return ret_val;
}
