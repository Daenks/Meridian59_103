// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * timer.c
 *
 This module maintains the min binary heap containing the Blakod timers.
 It also comtains the main loop of the program.
 */

#include "blakserv.h"

// Macros for accessing heap elements.
#define LCHILD(x) (2 * x + 1)
#define RCHILD(x) (2 * x + 2)
#define PARENT(x) ((x-1)/2)

static Bool in_main_loop = False;
static int numActiveTimers = 0;

// Min binary heap array of pointers of timer_node.
timer_node **timer_heap;
// Number of allocated timer nodes in timer_heap.
int num_timer_nodes;

// Next available timer ID.
int next_timer_num;

// If we pause timers during a save, keep track of the time here.
int pause_time;

/* local function prototypes */
void ReallocTimerNodes(void);
void TimerAddNode(timer_node *t);
void ResetLastMessageTimes(session_node *s);

int GetNumActiveTimers(void)
{
   return numActiveTimers;
}

#pragma region Timer Heap

// Swaps two timers (ID, obj ID, msg ID, firing time) by heap index.
__forceinline static void TimerSwapIndex(int i1, int i2)
{
   timer_node temp;

   temp.time = timer_heap[i1]->time;
   temp.timer_id = timer_heap[i1]->timer_id;
   temp.object_id = timer_heap[i1]->object_id;
   temp.message_id = timer_heap[i1]->message_id;

   timer_heap[i1]->time = timer_heap[i2]->time;
   timer_heap[i1]->timer_id = timer_heap[i2]->timer_id;
   timer_heap[i1]->object_id = timer_heap[i2]->object_id;
   timer_heap[i1]->message_id = timer_heap[i2]->message_id;

   timer_heap[i2]->time = temp.time;
   timer_heap[i2]->timer_id = temp.timer_id;
   timer_heap[i2]->object_id = temp.object_id;
   timer_heap[i2]->message_id = temp.message_id;
}

// Fixes the heap after a timer has been deleted or modified.
__inline void TimerHeapHeapify(int index)
{
   int i = index;
   while (i > 0 && timer_heap[i]->time < timer_heap[PARENT(i)]->time)
   {
      TimerSwapIndex(i, PARENT(i));
      i = PARENT(i);
   }
   do
   {
      int min = i;
      if (LCHILD(i) < numActiveTimers && timer_heap[LCHILD(i)]->time <= timer_heap[min]->time)
         min = LCHILD(i);
      if (RCHILD(i) < numActiveTimers && timer_heap[RCHILD(i)]->time < timer_heap[min]->time)
         min = RCHILD(i);
      if (min == i)
         break;
      TimerSwapIndex(i, min);
      i = min;
   } while (true);
}

// Removes a timer from the heap.
void TimerHeapRemove(int index)
{
   // Decrement heap size.
   --numActiveTimers;

   if (index == numActiveTimers)
   {
      // Node was a leaf, return.
      if (numActiveTimers <= 0)
      {
         numActiveTimers = 0;
      }
      return;
   }

   // Swap it with leaf.
   TimerSwapIndex(index, numActiveTimers);

   // Needs to be pushed down.
   TimerHeapHeapify(index);
}

// Adds a timer to the heap, using timer_node data. Passed
// timer_node is first unused element in timer_heap array.
__inline void TimerAddNode(timer_node *t)
{
   if (numActiveTimers == 0 || timer_heap[0]->time > t->time)
   {
      // We're making a new first-timer, so the time main loop should wait might
      // have changed, so have it break out of loop and recalibrate
      MessagePost(main_thread_id, WM_BLAK_MAIN_RECALIBRATE, 0, 0);
   }

   // Start node off at end of heap.
   int i = numActiveTimers++;
   timer_heap[i]->heap_index = i;

   // Push node up if necessary.
   while (i > 0 && timer_heap[i]->time < timer_heap[PARENT(i)]->time)
   {
      TimerSwapIndex(i, PARENT(i));
      i = PARENT(i);
   }
}

// Traverses the timer heap and returns false if the heap is invalid at any point.
bool TimerHeapCheck(int i, int level)
{
   if (i >= numActiveTimers)
      return true;

   if (LCHILD(i) < numActiveTimers && timer_heap[LCHILD(i)]->time < timer_heap[i]->time)
   {
      dprintf("TimerHeapCheck error on level %i", level);
      return false;
   }
   if (RCHILD(i) < numActiveTimers && timer_heap[RCHILD(i)]->time < timer_heap[i]->time)
   {
      dprintf("TimerHeapCheck error on level %i", level);
      return false;
   }

   bool retval = TimerHeapCheck(LCHILD(i), ++level);
   if (!retval)
      return false;

   return TimerHeapCheck(RCHILD(i), level);
}

#pragma endregion

// InitTimer should only be called during server startup (MainServer()) as
// this memory is not freed until the server exits.
void InitTimer(void)
{
   // Init timer heap
   num_timer_nodes = INIT_TIMER_NODES;
   timer_heap = (timer_node **)AllocateMemory(MALLOC_ID_TIMER,
      sizeof(timer_node *) * num_timer_nodes);
   for (int i = 0; i < num_timer_nodes; ++i)
      timer_heap[i] = (timer_node *)AllocateMemory(MALLOC_ID_TIMER, sizeof(timer_node));
   next_timer_num = 0;
   numActiveTimers = 0; // Keeps track of timers in heap
   
   pause_time = 0;
}

// Reallocates the timer heap memory if there is not enough space to create
// the next timer. Allocates timer_node memory at each new timer_heap element.
void ReallocTimerNodes(void)
{
   int old_timer_nodes = num_timer_nodes;

   num_timer_nodes = num_timer_nodes * 2;
   timer_heap = (timer_node **)ResizeMemory(MALLOC_ID_TIMER, timer_heap,
      old_timer_nodes * sizeof(timer_node *), num_timer_nodes * sizeof(timer_node *));
   lprintf("ReallocTimerNodes resized to %i timer nodes\n", num_timer_nodes);
   for (int i = old_timer_nodes; i < num_timer_nodes; ++i)
      timer_heap[i] = (timer_node *)AllocateMemory(MALLOC_ID_TIMER, sizeof(timer_node));
}

void ResetTimer(void)
{
   ClearTimer();
}

void ClearTimer(void)
{
   next_timer_num = 0;
   numActiveTimers = 0;
}

void PauseTimers(void)
{
   if (pause_time != 0)
   {
      eprintf("PauseTimers called when they were already paused at %s\n",TimeStr(pause_time));
      return;
   }
   pause_time = GetTime();
}

void UnpauseTimers(void)
{
   int add_time;

   if (pause_time == 0)
   {
      eprintf("UnpauseTimers called when they were not paused\n");
      return;
   }
   add_time = 1000*(GetTime() - pause_time);

   for (int i = 0; i < numActiveTimers; ++i)
      timer_heap[i]->time += add_time;

   pause_time = 0;
   
   /* after timers unpaused, we should reset last message times of people in the game
      so they aren't logged because of what seems to be lag */

   ForEachSession(ResetLastMessageTimes);
}

void ResetLastMessageTimes(session_node *s)
{
   if (s->state != STATE_GAME)
      return;

   s->game->game_last_message_time = GetTime();
}

int CreateTimer(int object_id,int message_id,int milliseconds)
{
   timer_node *t;

   if (numActiveTimers == num_timer_nodes)
      ReallocTimerNodes();

   t = timer_heap[numActiveTimers];
   t->timer_id = next_timer_num++;
   t->object_id = object_id;
   t->message_id = message_id;
   t->time = GetMilliCount() + milliseconds;

   TimerAddNode(t);

   return next_timer_num - 1;
}

Bool LoadTimer(int timer_id,int object_id,char *message_name,int milliseconds)
{
   object_node *o;
   timer_node *t;
   message_node *m;

   o = GetObjectByID(object_id);
   if (o == NULL)
   {
      eprintf("LoadTimer can't find object %i\n",object_id);
      return False;
   }

   m = GetMessageByName(o->class_id,message_name,NULL);
   if (m == NULL) 
   {
      eprintf("LoadTimer can't find message name %s\n",message_name);
      return False;
   }

   if (numActiveTimers == num_timer_nodes)
      ReallocTimerNodes();

   t = timer_heap[numActiveTimers];
   t->timer_id = timer_id;
   t->object_id = object_id;
   t->message_id = m->message_id;
   t->time = GetMilliCount() + milliseconds;

   TimerAddNode(t);

   /* the timers weren't saved in numerical order, but they were
    * compacted to first x non-negative integers
    */
   if (timer_id >= next_timer_num)
      next_timer_num = timer_id + 1;

   return True;
}

// Get timer by ID and remove it from heap by heap_index.
Bool DeleteTimer(int timer_id)
{
   timer_node *t;

   t = GetTimerByID(timer_id);
   
   if (t)
   {
      TimerHeapRemove(t->heap_index);

      return true;
   }

   eprintf("DeleteTimer can't find timer %i\n", timer_id);

#if 0
   // list the active timers.
   for (int i = 0; i < numActiveTimers; ++i)
   {
      dprintf("%i ",timer_heap[i]->timer_id);
   }
   dprintf("\n");
#endif

   return false;
}

/* activate the 1st timer, if it is time */
void TimerActivate()
{
   int object_id,message_id;
   UINT64 now;
   val_type timer_val;
   parm_node p[1];
   
   if (numActiveTimers == 0)
      return;
   
   now = GetMilliCount();
   if (now >= timer_heap[0]->time)
   {
   /*
     if (now - timers->time > TIMER_DELAY_WARN)
       dprintf("Timer handled %i.%03is late\n",
         (now-timers->time)/1000,(now-timers->time)%1000);
   */

      object_id = timer_heap[0]->object_id;
      message_id = timer_heap[0]->message_id;
      
      timer_val.v.tag = TAG_TIMER;
      timer_val.v.data = timer_heap[0]->timer_id;
      
      p[0].type = CONSTANT;
      p[0].value = timer_val.int_val;
      p[0].name_id = TIMER_PARM;

      TimerHeapRemove(0);

      SendTopLevelBlakodMessage(object_id,message_id,1,p);
   }
}

Bool InMainLoop(void)
{
   return in_main_loop;
}

void ServiceTimers(void)
{
   MSG msg;
   INT64 ms;

   StartupComplete(); /* for the interface to report no errors on startup */
#ifdef BLAK_PLATFORM_WINDOWS
   InterfaceUpdate();
#endif
   lprintf("Status: %i accounts\n",GetNextAccountID());

   lprintf("-------------------------------------------------------------------------------------\n");
   dprintf("-------------------------------------------------------------------------------------\n");
   eprintf("-------------------------------------------------------------------------------------\n");

   in_main_loop = True;

#ifdef BLAK_PLATFORM_WINDOWS
   SetWindowText(hwndMain, ConfigStr(CONSOLE_CAPTION));
#endif

   AsyncSocketStart();

   for(;;)
   {
      if (numActiveTimers == 0)
         ms = 500;
      else
      {
         ms = timer_heap[0]->time - GetMilliCount();
         if (ms <= 0)
            ms = 0;

         if (ms > 500)
            ms = 500;
      }	 
      
      if (WaitForAnyMessageWithTimeout(ms))
      {
         while (MessagePeek(&msg,NULL,0,0,PM_REMOVE))
         {
            if (msg.message == WM_QUIT)
            {
               lprintf("ServiceTimers shutting down the server\n");   
               return;
            }
	    
            switch (msg.message)
            {
               case WM_BLAK_MAIN_READ :
               EnterServerLock();

               PollSession(msg.lParam);
               TimerActivate();

               LeaveServerLock();
               break;

               case WM_BLAK_MAIN_RECALIBRATE :
               /* new soonest timer, so we should recalculate our time left... 
               so we just need to restart the loop! */
               break;

               case WM_BLAK_MAIN_DELETE_ACCOUNT :
               EnterServerLock();
               DeleteAccountAndAssociatedUsersByID(msg.lParam);
               LeaveServerLock();
               break;

               case WM_BLAK_MAIN_VERIFIED_LOGIN :
               EnterServerLock();
               VerifiedLoginSession(msg.lParam);
               LeaveServerLock();
               break;

               case WM_BLAK_MAIN_LOAD_GAME :
               EnterServerLock();
               LoadFromKod(msg.lParam);
               LeaveServerLock();
               break;

               default :
               dprintf("ServiceTimers got unknown message %i\n",msg.message);
               break;
            }
         }
      }
      else
      {
         /* a Blakod timer is ready to go */
         EnterServerLock();
         PollSessions(); /* really just need to check session timers */
         TimerActivate();
         LeaveServerLock();
      }
   }
}

// Iterate through timer_heap array and compare timer_id.
timer_node * GetTimerByID(int timer_id)
{
   if (numActiveTimers == 0)
      return NULL;

   for (int i = 0; i < numActiveTimers; ++i)
   {
      if (timer_heap[i]->timer_id == timer_id)
         return timer_heap[i];
   }
   return NULL;
}

void ForEachTimer(void (*callback_func)(timer_node *t))
{
   if (numActiveTimers == 0)
      return;

   for (int i = 0; i < numActiveTimers; ++i)
      callback_func(timer_heap[i]);
}

/* functions for garbage collection */

void SetNumTimers(int new_next_timer_num)
{
   next_timer_num = new_next_timer_num;
}
