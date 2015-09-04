// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
/*
* threading-linux.c
*

  This module contains threading and mutex wrapper functions to handle
     certain windows calls on a linux platform.

*/
 

#include "blakserv.h"

pthread_mutex_t** mutexes;
int num_mutexes = 0;

MSGQUEUE main_thread_msgs;

BOOL InitMsgQueue()
{
    main_thread_msgs.head = -1;
    main_thread_msgs.tail = -1;
    main_thread_count = 0;
    pthread_mutex_init(&main_thread_msgs.mux, NULL);
    pthread_cond_init(&main_thread_msgs.signal, NULL);
}

BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    // TODO: stub
    if (idThread != main_thread_id)
    {
        eprintf("PostThreadMessage: only main thread messages currently supported on Linux!");
        return FALSE;
    }

    // lock mutex
    // if zero messages
    //   copy message into queue
    //   set head to 0 set tail to 0
    // else (already messages)
    //   check if ring buffer is full
    //   if count >= MAX_MESSAGES
    //     error! message overflow!!!
    //   else (we have more room)
    //     if (tail < (MAX_MESSAGES - 1))
    //       if (head < tail)
    //       else (head > tail)
    //     else if (tail == (MAX_MESSAGES - 1))
    //       flip tail around to the beginning of the buffer
    //       tail = 0; 
    // unlock mutex

    return FALSE;
}

BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
    // lock mutex
    // if (count > 0)
    //  copy head into lpMsg
    //  decrement count
    //  if count > 0
    //   set head to head +1
    //   if head > (MAX_MESSAGES - 1)
    //    set head to
    //   unlock mutex
    //   return true
    //  else
    //   set head to -1
    //   set tail to -1
    //   unlock mutex
    //   return true
    // else
    //  unlock mutex
    //  return false (no messages)
    
    // TODO: Implement

    return FALSE;

}



// WARNING:  A very poor assumption is made here that the "object" type is a mutex.
// At this point in time that appears to be exactly how this function is being
// called in all cases.  The original windows function can wait for a list of several
// objects such as semaphores, events, and processes.  This function will clearly not
// be able to handle objects like that (nor will the code compile unless someone in
// the future implements those objects for linux - Matt (keen)
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += (dwMilliseconds  / 1000);
    ts.tv_nsec += ( dwMilliseconds - (( dwMilliseconds  / 1000 ) * 1000 )) * 1000000;

    return pthread_mutex_timedlock( (pthread_mutex_t*) hHandle, &ts);

}


DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *Handles, BOOL bWaitAll, DWORD  dwMilliseconds)
{
    // TODO: stub
    return 0;
}

DWORD MsgWaitForMultipleObjects( DWORD nCount, const HANDLE *Handles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask)
{
    // TODO: stub
    return 0;
}

// TODO: extend this to implement mutex names and attributes
HANDLE CreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName)
{
    if (num_mutexes > 0)
    {
        mutexes = (pthread_mutex_t**) realloc(mutexes, sizeof(pthread_mutex_t*) * num_mutexes + 1);
    }
    else
    {
        mutexes = (pthread_mutex_t**) malloc(sizeof(pthread_mutex_t*));
    }
    
    mutexes[num_mutexes] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutexes[num_mutexes], NULL);

    num_mutexes++;
   
    return mutexes[num_mutexes-1];
}

BOOL ReleaseMutex(HANDLE hMutex)
{
    int count = 0;
    pthread_mutex_t* mutex_to_destroy;
    int retVal = EINVAL;

    if (num_mutexes > 0)
    {
        while ((void*) mutexes[count] != hMutex)
        {
            ++count;
        }

        mutex_to_destroy = (pthread_mutex_t*) mutexes[count];

        while(count < num_mutexes)
        {
            if (count > 0)
                mutexes[count - 1] = mutexes[count];

            ++count;
        }
        num_mutexes--;

        mutexes = (pthread_mutex_t**) realloc(mutexes, sizeof(pthread_mutex_t*) * num_mutexes + 1);
        retVal = pthread_mutex_destroy(mutex_to_destroy);
        free(mutex_to_destroy);

    }

    return retVal;
}
