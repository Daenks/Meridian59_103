// 
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

typedef struct POINT {
  long x;
  long y;
} POINT, *PPOINT;

// Windows ttype MSG struct for portability
typedef struct MSG {
  HWND   hwnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
  POINT  pt;
} MSG, *PMSG, *LPMSG;

// Wrapper to turn MSG to a list
typedef struct MsgNode
{
    MsgNode* next;
    MsgNode* prev;
    MSG     msg;
} MsgNode;

#define MSGBUFSIZE 256

// simulate windows message queues
 typedef struct MSGQUEUE {
   MsgNode* head;
   MsgNode* tail;
   int count;
   pthread_mutex_t mux;
   pthread_cond_t msgEvent;
} MSGQUEUE;

BOOL InitMsgQueue();
BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
//DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
DWORD MsgWaitForMultipleObjects( DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask);
HANDLE CreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName);
BOOL ReleaseMutex(HANDLE hMutex);

#endif
