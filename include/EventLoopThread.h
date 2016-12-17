#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H


#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"

#include <boost/noncopyable.hpp>

class EventLoop;

class EventLoopThread : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const string& name = string());
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* M_loop;  
    bool M_exiting;
    Thread M_thread;
    MutexLock M_mutex;
    Condition M_cond;
    ThreadInitCallback M_callback;              
};


#endif