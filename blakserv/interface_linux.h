// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#ifndef _INTERFAC_LINUX_H
#define _INTERFAC_LINUX_H

#define FatalError(a) FatalErrorShow(__FILE__,__LINE__,a)

void InitInterface(void);
void* InterfaceMainLoop(void*);

void StartupPrintf(const char *fmt,...);
void StartupComplete(void);

int GetUsedSessions(void);

void InterfaceUpdate(void);
void InterfaceLogon(session_node *s);
void InterfaceLogoff(session_node *s);
void InterfaceUpdateSession(session_node *s);
void InterfaceUpdateChannel(void);

void InterfaceSendBufferList(buffer_node *blist);
void InterfaceSendBytes(char *buf,int len_buf);

void FatalErrorShow(const char *filename,int line,const char *str);

#endif

