// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * timer.h
 *
 */

#ifndef _TIMER_H
#define _TIMER_H

#define INIT_TIMER_NODES (2000)

typedef struct timer_struct
{
   UINT64 time;
   int timer_id;
   int object_id;
   int message_id;
   int garbage_ref;
   int heap_index;
} timer_node;

bool TimerHeapCheck(int i, int level);
void InitTimer(void);
void ResetTimer(void);
void ClearTimer(void);
void PauseTimers(void);
void UnpauseTimers(void);
int CreateTimer(int object_id,int message_id,int milliseconds);
Bool LoadTimer(int timer_id,int object_id,char *message_name,int milliseconds);
Bool DeleteTimer(int timer_id);
void ServiceTimers(void);
void QuitTimerLoop(void);
timer_node * GetTimerByID(int timer_id);
void ForEachTimer(void (*callback_func)(timer_node *t));
void SetNumTimers(int new_next_timer_num);
Bool InMainLoop(void);
int  GetNumActiveTimers(void);

#endif
