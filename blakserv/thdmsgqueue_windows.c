// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
 
#include "blakserv.h"
                 
bool WaitForAnyMessageWithTimeout(INT64 ms)
{
   return (MsgWaitForMultipleObjects(0,NULL,0,(DWORD)ms,QS_ALLINPUT) == WAIT_OBJECT_0);
}

BOOL MessagePeek(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
   return PeekMessage(lpMsg,hWnd,wMsgFilterMin,wMsgFilterMax,wRemoveMsg);
}

BOOL MessagePost(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
{
   return PostThreadMessage(idThread,Msg,wParam,lParam);
}
