// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#ifndef _TMSGLOOP_WINDOWS_H
#define _TMSGLOOP_WINDOWS_H

bool WaitForAnyMessageWithTimeout(INT64 ms);
BOOL MessagePeek(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL MessagePost(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);

#endif
