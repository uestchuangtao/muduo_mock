#include <EventLoopTHreadPool.h>

#include <EventLoop.h>
#include <EventLoopThread.h>

#include <boost/bind.hpp>

#include <stdio.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg)
    : M_baseLoop(baseLoop),
      M_name(nameArg),
      M_started(false),
      M_numThreads(0),
      M_next(0)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{
    //Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    assert(!M_started);
    M_baseLoop->assertInLoopThread();

    M_started = true;

    for(int i = 0; i < M_numThreads; ++i)
    {
        char buf[name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        M_threads.push_back(t);
        M_loops.push_back(t->startLoop());
    }

    if(M_numThreads == 0 && cb)
    {
        cb(M_baseLoop);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    M_baseLoop->assertInLoopThread();
    assert(M_started);
    EventLoop* loop = M_baseLoop;

    if(!M_loops.empty())
    {
        //round-robin
        loop = M_loops[M_next];
        ++M_next;
        if(implicit_cast<size_t>(M_next) >= M_loops.size())
        {
            M_next = 0;
        }
        return loop;
    }
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
    M_baseLoop->assertInLoopThread();
    EventLoop* loop = M_baseLoop;

    if(!M_loops.empty())
    {
        loop = M_loops[hashCode % M_loops.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    M_baseLoop->assertInLoopThread();
    assert(M_started);
    if(M_loops.empty())
    {
        return std::vector<EventLoop*>(1,M_baseLoop);
    }
    else
    {
        return M_loops;
    }
}