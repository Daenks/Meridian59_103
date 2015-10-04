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

MSGQUEUE Msgs;

BOOL InitMsgQueue()
{
    Msgs.head = NULL;
    Msgs.count = 0;
    pthread_mutex_init(&Msgs.mux, NULL);
    pthread_cond_init(&Msgs.msgEvent, NULL);
}

BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    // TODO: Review
    MsgNode* temp;

    if (idThread != main_thread_id)
    {
        eprintf("PostThreadMessage: only main thread messages currently supported on Linux!");
        return FALSE;
    }

    pthread_mutex_lock(&Msgs.mux);

    if (Msgs.head == NULL)
    {
        Msgs.head = (MsgNode*)malloc(sizeof(MsgNode));
        Msgs.head->next = NULL;
        Msgs.tail->prev = NULL;
        Msgs.tail = Msgs.head;
    }
    else
    {
        Msgs.tail->next = (MsgNode*)malloc(sizeof(MsgNode));
        Msgs.tail->next->prev = Msgs.tail;
        Msgs.tail->next->next = NULL;
        Msgs.tail = Msgs.tail->next;
    }

    Msgs.tail->msg.hwnd = (HWND)idThread;
    Msgs.tail->msg.message = Msg;
    Msgs.tail->msg.lParam = lParam;
    Msgs.tail->msg.wParam = wParam;

    Msgs.count++;

    pthread_cond_signal(&Msgs.msgEvent);
    pthread_mutex_unlock(&Msgs.mux);

    return TRUE;
}

BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
    // TODO: Finish
    // TODO: handle wRemoveMsg
    MsgNode* current;

    pthread_mutex_lock(&Msgs.mux);

    if (Msgs.head == NULL)
    {
        return FALSE;
    }

    current = Msgs.head;

    while(current != NULL && 
            !((current->msg.hwnd == hWnd) &&
                (current->msg.message >= wMsgFilterMin &&
                 current->msg.message <= wMsgFilterMax)))
    {
        current = current->next;
    }

    if (current == NULL)
    {
        return FALSE;
    }

    lpMsg = (MSG*)malloc(sizeof(MSG));
    lpMsg->hwnd = current->msg.hwnd;
    lpMsg->message = current->msg.message;
    lpMsg->lParam = current->msg.lParam;
    lpMsg->wParam = current->msg.wParam;

    pthread_mutex_unlock(&Msgs.mux);

    return TRUE;

}



// WARNING:  A very poor assumption is made here that the "object" type is a mutex.
// At this point in time that appears to be exactly how this function is being
// called in all cases.  The original windows function can wait for several types of
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


//DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE *Handles, BOOL bWaitAll, DWORD  dwMilliseconds)
//{
//    // TODO: stub
//    return 0;
//}

DWORD MsgWaitForMultipleObjects( DWORD nCount, const HANDLE *Handles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask)
{
    // TODO: stub, incomplete

    pthread_mutex_lock(&Msgs.mux);
    pthread_cond_wait(&Msgs.msgEvent, &Msgs.mux);

    // cond_wait will unlock the mutex while waiting then lock it when it returns, we must unlock it now
    pthread_mutex_unlock(&Msgs.mux);

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
