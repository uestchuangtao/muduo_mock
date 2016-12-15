#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include "Condition.h"
#include "Mutex.h"

#include <boost/noncopyable.hpp>
#include <deque>
#include <assert.h>

template<typename T>
class BlockingQueue : boost::noncopyable
{
public：
    BlockingQueue()
        :M_mutex(),
         M_notEmpty(M_mutex),
         M_queue()
    {
    }

    void put(const T& x)
    {
        MutexLockGuard(M_mutex);
        M_queue.push_back(x);
        M_notEmpty.notify();
    }

    T take()
    {
        MutexLockGuard(M_mutex);
        while(M_queue.empty())
        {
            M_notEmpty.wait();
        }
        assert(!M_queue.empty());

        T front(M_queue.front());
        M_queue.pop_front();
        return front;
    }

    size_t size() const
    {
        MutexLockGuard(M_mutex);
        return M_queue,size();
    } 

private:
    mutable MutexLock M_mutex;
    Condition M_notEmpty;
    std::deque<T> M_queue;
};

#endif