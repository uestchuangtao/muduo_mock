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

}