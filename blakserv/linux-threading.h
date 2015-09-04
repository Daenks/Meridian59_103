// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
/*
* threading-linux.h
*

  This module contains threading and mutex wrappera functions to handle
     certain windows calls on a linux platform.

*/


#ifndef _PTHREADING_H
#define _PTHREADING_H

// Linux version of Windows mutex and various threading functions, using pthreads.

#include <pthread.h>

typedef struct _SECURITY_ATTRIBUTES {
  DWORD  nLength;
  LPVOID lpSecurityDescriptor;
  BOOL   bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct tagPOINT {
  long x;
  long y;
} POINT, *PPOINT;

typedef struct tagMSG {
  HWND   hwnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
  POINT  pt;
} MSG, *PMSG, *LPMSG;

#define MSGBUFSIZE 256

// simulate windows message queues
 typedef struct tagMSGQUEUE {
   int head;
   int tail;
   int count;
   MSG MsgBuf[MSGBUFSIZE];
   pthread_mutex_t mux;
   pthread_cond_t signal;
} MSGQUEUE;

BOOL InitMsgQueue();
BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
DWORD MsgWaitForMultipleObjects( DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask);
HANDLE CreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName);
BOOL ReleaseMutex(HANDLE hMutex);

#endif
