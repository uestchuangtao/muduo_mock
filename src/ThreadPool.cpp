#include "ThreadPool.h"
#include "Exception.h"

#include <boost/bind.hpp>
#include <assert.h>
#include <stdio.h>

ThreadPool::ThreadPool(const string& nameArg)
    :M_mutex(),
     M_notEmpty(M_mutex),
     M_notFull(M_mutex),
     M_name(nameArg),
     M_maxQueueSize(0),
     M_running(false)
{    
}

ThreadPool::~ThreadPool()
{
    if(M_running)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    assert(M_threads.empty());
    M_running = true;
    M_threads.reserve(numThreads);
    for(int i = 0; i < numThreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof(id), "%d", i+1)；
        M_threads.push_back(new Thread(boost::bind(&ThreadPool::runInThread, this), M_name+id));
        M_threads[i].start();
    }

    if(numThreads == 0 && M_threadInitCallback)
    {
        M_threadInitCallback();
    }
}

void ThreadPool::stop()
{
    
    {
        MutexLockGuard lock(M_mutex);
        M_running = false;
        M_notEmpty.notifyAll();
    }
    for_each(M_threads.begin(),
             M_threads.end(),
             boost::bind(&Thread::join,_1));
             
}

size_t ThreadPool::queueSize() const 
{
    MutexLockGuard lock(M_mutex);
    return M_queue.size();
}

void ThreadPool::run(const Task& task)
{
    if(M_threads.empty())
    {
        task();
    }
    else
    {
        MutexLockGuard lock(M_mutex);
        while(isFull())
        {
            M_notFull.wait();
        }
        assert(!isFull());

        M_queue.push_back(task);

        M_notEmpty.notify();
    }
}

ThreadPool::Task ThreadPool::take()
{
    MutexLockGuard lock(M_mutex);
    
}


     