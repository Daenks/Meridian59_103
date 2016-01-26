// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#ifndef _TMSGLOOP_WINDOWS_H
#define _TMSGLOOP_WINDOWS_H

//typedef struct _SECURITY_ATTRIBUTES {
//  DWORD  nLength;
//  LPVOID lpSecurityDescriptor;
//  BOOL   bInheritHandle;
//} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

//typedef struct POINT {
//  long x;
//  long y;
//} POINT, *PPOINT;

// Windows ttype MSG struct for portability
typedef struct MSG {
  HWND   hwnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
  //POINT  pt;
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

bool InitMsgQueue();
//BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);
//BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
bool WaitForAnyMessageWithTimeout(INT64 ms);
bool MessagePeek(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
bool MessagePost(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);

#endif
