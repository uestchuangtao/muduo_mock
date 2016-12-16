#include "EventLoop.h"

#include "Logging.h"
#include "Mutex.h"
#include "Channel.h"
#include "Poller.h"
#include "SocketsOps.h"
#include "TimerQueue.h"

#include <boost/bind.hpp>

#include <signal.h>
#include <sys/eventfd.h>

namespace
{
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;

EventLoop::EventLoop()
    : M_looping(false),
      M_quit(false),
      M_eventHandling(false),
      M_callingPendingFunctors(false),
      M_iteration(0),
      M_threadId(CurrentThread::tid()),
      M_poller(Poller::newDefaultPoller(this)),
      M_timerQueue(new TimerQueue(this)),
      M_wakeupFd(createEventfd()),
      M_wakeupChannel(new Channel(this, M_wakeupFd)),
      M_currentActiveChannel(NULL)
{
    LOG_DEBUG << "EventLoop created "<< this << "in thread" << M_threadId;
    if(t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << "exists in this thread " << M_threadId;
    }
    else 
    {
        t_loopInThisThread = this；
    }
    M_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //we are always reading the wakeupfd
    M_wakeupChannel->enableReading();

}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << "of thread " <<  M_threadId
              << "destructs in thread " << CurrentThread::tid();
    M_wakeupChannel->disableAll();
    M_wakeupChannel->remove();
    ::close(M_wakeupFd);
    t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
    assert(!M_looping);
    assertInLoopThread();
    M_loop = true;
    M_quit = false;
    LOG_TRACE << "EventLoop " << this << "start looping";

    while(!M_quit)
    {
        M_activeChannels.clear();
        M_pollReturnTime = M_poller->poll(kPollTimeMs, &M_activeChannels);
        ++M_iteration;
        if(Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }
        // TODO sort channel by priority
        M_eventHandling = true;
        for(ChannelList::iterator it = M_activeChannels.begin();
            it != M_activeChannels.end(), ++it)
        {
            M_currentActiveChannel = *it;
            M_currentActiveChannel->handleEvent(M_pollReturnTime);
        }
        M_currentActiveChannel = NULL;
        M_eventHandling = false;
        doPendingFunctors();
    }

    LOG_TRACE << "EventLoop " << this << "stop looping";
    M_looping = false;
}

void EventLoop::quit()
{
    M_quit = true;
    //There is  a chance that loop() just executes while(!M_quit) and exits,
    //then EventLoop destructs, then we are accessing an invalid objects. 
    //Can be fixed using M_mutex in both places. 
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(const Functor& cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb)
{
    {
        MutexLockGuard lock(M_mutex);
        M_pendingFunctors.push_back(cb);
    }

    if(!isInLoopThread() || M_callingPendingFunctors)
    {
        wakeup();
    }
}

size_t EventLoop::queueSize() const 
{
    MutexLockGuard lock(M_mutex);
    return M_pendingFunctors.size();
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return M_timerQueue->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(),interval));
    return M_timerQueue->addTimer(cb， time);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return M_timerQueue->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return M_timerQueue->cancel(TimerId);
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    M_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel * channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if(M_eventHandling)
    {
        assert(M_currentActiveChannel == channel ||
               std::find(M_activeChannels.begin(),M_activeChannels.end(), channel) == M_activeChannels.end());
    }
    M_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return M_poller->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << M_threadId
            << ", current thread id = " <<  CurrentThread::tid();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = sockets::write(M_wakeupFd, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::wakeup() writes "<< n << <<" bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = sockets::read(M_wakeupFd, &one, sizeof(one));
    if( n ！= sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads "<< n << "bytes instead of 8"；
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    M_callingPendingFunctors = true;
    {
        MutexLockGuard lock(M_mutex);
        functors.swap(M_pendingFunctors);
    }
    for(size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    M_callingPendingFunctors = false;
}

void EventLoop::printActiveChannels() const 
{
    for(ChannelList::const_iterator it = M_activeChannels.begin();
        it != M_activeChannels.end(); ++it)
    {
        const Channel *ch = *it;
        LOG_TRACE << "{" << ch->reventsToString() << "}";
    }    
}

}

