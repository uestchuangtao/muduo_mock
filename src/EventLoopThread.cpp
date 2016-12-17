#include "EventLoopThread.h"

#include "EventLoop.h"

#include <boost/bind.hpp>

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const string& name)
    : M_loop(NULL),
      M_exiting(false),
      M_thread(boost::bind(&EventLoopThread::threadFunc, this), name),
      M_mutex(),
      M_cond(M_mutex),
      M_callback(cb)
{

}   

EventLoopThread::~EventLoopThread()
{
    M_exiting = true;
    if(M_loop != NULL)  //not 100% race-free, eg. threadFunc could be running callback
    {
        //still a tiny chance to call destructed object, if threadFunc exits just now.
        //but when EventLoopThread destructs, usually programming is exiting anyway. 
        M_loop->quit();
        M_thread.join();
    }
} 

EventLoop* EventLoopThread::startLoop()
{
    assert(!M_thread.started());
    M_thread.start();

    {
        MutexLockGuard lock(M_mutex);
        while(loop == NULL)
        {
            M_cond.wait();
        }
    }
    return M_loop;
} 

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if(M_callbck)
    {
        M_callback(&loop);
    }

    {
        MutexLockGuard lock(M_mutex);
        M_loop = &loop;
        M_cond.notify();
    }

    loop.lop();

    M_loop = NULL;
}                                 