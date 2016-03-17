// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#ifndef _ASYNC_WINDOWS_H
#define _ASYNC_WINDOWS_H
void StartAsyncSocketAccept(SOCKET sock,int connection_type);
HANDLE StartAsyncNameLookup(char *peer_addr,char *buf);
void StartAsyncSession(session_node *s);
void AsyncSocketAccept(SOCKET sock,int event,int error,int connection_type);
void AsyncSocketSelect(SOCKET sock,int event,int error);

#endif
