// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* queue.h   Threadsafe implementation of a queue using cx 11 ::std:queue and ::std::mutex
*
*/

#ifndef _QUEUE_H
#define _QUEUE_H

#include <queue>
#include <mutex>

template <typename T>
class threadsafe_queue
{
private:
   ::std::queue<T> queue;
   ::std::mutex mutex;
   unsigned int maxsize;

public:
   threadsafe_queue(int size)
   {
      maxsize = size;
   }

   ~threadsafe_queue()
   {
   }

   bool enqueue(T item)
   {
      bool ret = false;

      mutex.lock();
      if (queue.size() < maxsize)
      {
         ret = true;
         queue.push(item);
      }
      mutex.unlock();

      return ret;
   }

   T dequeue()
   {
      T ret = NULL;

      mutex.lock();
      if (!queue.empty())
      {
         ret = queue.front();
         queue.pop();
      }
      mutex.unlock();

      return ret;
   }

   bool empty()
   {
      bool ret = true;

      mutex.lock();
      ret = queue.empty();
      mutex.unlock();

      return ret;
   }
};
#endif /*#ifndef _QUEUE_H */
