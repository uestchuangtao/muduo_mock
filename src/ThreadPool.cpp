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
    //always use a while-loop, due to spurious wakeup
    while(M_queue.empty() && M_running)
    {
        M_notEmpty.wait();
    }
    Task task;
    if(!M_queue.empty())
    {
        task = M_queue.front();
        M_queue.pop_front();
        if(M_maxQueueSize > 0)
        {
            M_notFull.notify();
        }
    }
    return task;
}

bool ThreadPool::isFull() const 
{
    M_mutex.assertLocked();
    return M_maxQueueSize > 0 && M_queue.size() >= M_maxQueueSize;
}

void ThreadPool::runInThread()
{
    try
    {
        if(M_threadInitCallback)
        {
            M_threadInitCallback();
        }
        while(M_running)
        {
            Task task(take());
            if(task)
            {
                task();
            }
        }
    }
    catch (const Exception &ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", M_name.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception &ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", M_name.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", M_name.c_str());
        throw; // rethrow
    }
}


     