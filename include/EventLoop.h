#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "Mutex.h"
#include "CurrentThread.h"
#include "TimeStamp.h"
#include "Callbacks.h"
#include "TimerId.h"


class Channel;
class Poller;
class TimerQueue;

///
///Reactor, at most one per thread.  
///
///This is an interface class, so don't expose too much details.
class EventLoop : boost::noncopyable 
{
public:
    typedef boost::function<void()> Functor;

    EventLoop();
    ~EventLoop();  //force out-line dtor, for scoped_ptr members. 

    ///
    /// Loops forever. 
    /// 
    ///Must be called in the same thread as Creation of the objects. 
    void loop();

    ///Quits loop
    ///
    ///This is not 100% thread safe, if you call through a raw pointer,
    ///better to call through shared_ptr<EventLoop> for 100% safety. 
    void quit();

    ///
    ///Time when poll returns, usually means data arrival.
    ///
    Timestamp pollReturnTime() const 
    {
        return M_pollReturnTime;
    }

    int64_t iteration() const 
    {
        return M_iteration;
    }

    /// Runs callback immediately in the loop thread.
    /// It wakes up the loop, and run the cb.
    /// If in the same loop thread, cb is run within the function.
    /// Safe to call from other threads.
    void runInLoop(const Functor& cb);

    /// Queues callback in the loop thread.
    /// Runs after finish pooling.
    /// Safe to call from other threads.
    void queueInLoop(const Functor &cb);

    size_t queueSize() const;

    //timers

    ///
    /// Runs callback at 'time'. 
    /// Safe to call from other threads.
    ///
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);

    ///
    /// Runs callback after @c delay seconds.
    /// Safe to call from other threads.
    ///
    TimerId runAfter(double delay, const TimerCallback& cb)；

    ///
    /// Runs callback every @c interval seconds. 
    /// Safe to call from other threads. 
    ///
    TimerId runEvery(double interval, const TimerCallback& cb);

    ///
    /// Cancels the timer. 
    /// Safe to call from other threads.
    ///
    void cancel(TimerId timerId);


    // internal usage
    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    void assertInLoopThread()
    {
        if(!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const 
    {
        return M_threadId == CurrentThread::tid();
    }

    bool eventHandling() const 
    {
        return M_eventHandling;
    }

    void setContext(const boost::any& context)
    {
        M_context = context;
    }

    const boost::any& getContext() const 
    {
        return M_context;
    }

    boost::any* getMutableContext()
    {
        return &M_context;
    }

    static EventLoop* getEventLoopOfCurrentThread();

  private:
    void abortNotInLoopThread();
    void handleRead(); //waked up
    void doPendingFunctors();

    void printActiveChannels() const; //DEBUG

    typedef std::vector<Channel*> ChannelList;

    bool M_looping;
    bool M_quit;
    bool M_eventHandling;
    bool M_callingPendingFunctors;
    int64_t M_iteration;
    const pid_t M_threadId;
    Timestamp M_pollReturnTime;
    boost::scoped_ptr<Poller> M_poller;
    boost::scoped_ptr<TimerQueue> M_timerQueue;
    int M_wakeupFd;
    //unlike in TimerQueue, which is an internal class,
    //we don't expose Channel to client. 
    boost::scoped_ptr<Channel> M_wakeupChannel;
    boost::any M_context;

    //scratch variables
    ChannelList M_activeChannels;
    Channel* M_currentActiveChannel;

    mutable MutexLock M_mutex;
    std::vector<Functor> M_pendingFunctors;
};

#endif