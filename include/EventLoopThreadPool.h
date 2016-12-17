#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include "Types.h"

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads)
    {
        M_numThreads = numThreads;
    }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // valid after calling start()
    /// round-robin
    EventLoop* getNextLoop();

    /// with the same hash code, it will always return the same EventLoop
    EventLoop* getLoopForHash(size_t hashCode);

    std::vector<EventLoop*> getAllLoops();

    bool started() const 
    {
        return M_started;
    }

    const string& name() const 
    {
        return M_name;
    }

private:

    EventLoop* M_baseLoop;
    string M_name;
    bool M_started;
    int M_numThreads;
    int M_next;
    boost::ptr_vector<EventLoopThread> M_threads;
    std::vector<EventLoop*> M_loops;
};

#endif