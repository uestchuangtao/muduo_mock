#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
#include "Types.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

class ThreadPool : boost::noncopyable
{
public:
    typedef boost::function<void ()> Task;

    explicit ThreadPool(const string& nameArg = string("ThreadPool"));
    ~ThreadPool();

    //Must be called before start().
    void setMaxQueueSize(int maxSize)
    {
        M_maxQueueSize = maxsize;
    }    

    void setThreadInitCallback(const Task& cb)
    {
        M_threadInitCallback = cb;
    }

    void start(int numThreads);
    void stop();

    const string& name() const 
    {
        return M_name;
    }

    size_t queueSize() const;

    // Could block if maxQueueSize > 0
    void run(const Task& f);

private:
    bool isFull() const;
    void runInThread();
    Task take();

    mutable MutexLock M_mutex;
    Condition M_notEmpty;
    Condition M_notFull;
    string M_name;
    Task M_threadInitCallback;
    boost::ptr_vector<Thread> M_threads;
    std::deque<Task> M_queue;
    size_t M_maxQueueSize;
    bool M_running;
};

#endif