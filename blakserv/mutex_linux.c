// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.

#include "blakserv.h"

Mutex MutexCreate() {
   // TODO: this is probably not the best way to allocate the memory
   Mutex mutex = new MutexWrapper();
   pthread_mutex_init(&mutex->mutex, NULL);

   return mutex;
}

bool MutexAcquire(Mutex mutex) {
   return pthread_mutex_lock(&mutex->mutex) == 0;
}

bool MutexAcquireWithTimeout(Mutex mutex, int ms) {

   timespec ts;
   int retVal;

   clock_gettime(CLOCK_REALTIME, &ts);
   ts.tv_sec += (ms / 1000L);
   ts.tv_nsec += ( ms - (( ms  / 1000L ) * 1000L )) * 1000000L;

   // check for tv_nsec overflow (no more than 1,000,000,000)
   if (ts.tv_nsec > 1000000000)
   {
      ts.tv_nsec -= 1000000000;
      ts.tv_sec += 1;
   }

   retVal =  pthread_mutex_timedlock(&mutex->mutex, &ts);

   if (retVal != 0)
   {
      eprintf("MutexAcquireWithTimeout error %d",retVal);
      return false;
   }

   return true;
}

bool MutexRelease(Mutex mutex) {
   return pthread_mutex_unlock(&mutex->mutex) == 0;
}

bool MutexClose(Mutex mutex) {
   // TODO: delete() &mutex->mutex?
   return pthread_mutex_destroy(&mutex->mutex) == 0;
}
