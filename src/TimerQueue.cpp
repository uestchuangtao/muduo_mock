#include "TimerQueue.h"

#include "Logging.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"

#include <boost/bind.hpp>

#include <sys/timerfd.h>


namespace detail 
{
int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);  
    if(timerfd < 0)
    {
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;                
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                           - Timestamp::now.microSecondsSinceEpoch();
    if( microseconds < 100)
    {
        microseconds  = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds < 100;
    )
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    
    return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if(n != sizeof(howmany))
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << "bytes instead of 8";
    }
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if(ret)
    {
        LOG_SYSERR << "timerfd_settime()";
    }
}

}

using namespace detail;

TimerQueue::TimerQueue(EventLoop* loop)
    :M_loop(loop),
     M_timerfd(createTimerfd()),
     M_timerfdChannel(loop, M_timerfd),
     M_timers(),
     M_callingExpiredTimers(false)
{
    M_timerfdChannel.setReadCallback(
        boost::bind(&TimerQueue::handleRead, this));
    //we are always reading the timerfd, we disarm it with timerfd_settime. 
    M_TimerfdChannel.enableReading();
}

TimerQueue::~TimerQueue()
{
    M_timerfdChannel.disableAll();
    M_timerfdChannel.remove();
    ::close(M_timerfd);
    for(TimerList::iterator it = M_timers.begin();
        it != M_timers.end(); ++it)
    {
        delete it->second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback& cb,
                             Timestamp when,
                             double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    M_loop->runInLoop(boost::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    M_loop->runInLoop(boost::bind(&TimerQueue::cancelInLoop, this, timerId));
} 

void TimerQueue::addTimerInLoop(Timer* timer)
{
    M_loop->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if(earliestChanged)
    {
        resetTimerfd(M_timerfd, timer->expiration());
    }
}                           

void TimerQueue::cancelInLoop(TimerId timerId)
{
    M_loop->assertInLoopThread();
    assert(M_timers.size() == M_activeTimers.size());
    ActiveTimer timer(timerId.M_timer,timerId.M_sequence);
    ActiveTimerSet::iterator it = M_activeTimers.find(timer);
    if(it != M_activeTimers.end())
    {
        size_t n = M_timers.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first; //FIXME : no delete please
        M_activeTimers.erase(it);
    }
    else if(M_callingExpiredTimers)
    {
        M_cancelingTimers.insert(timer);
    }
    assert(M_timers.size() == M_activeTimers.size());
}

void TimerQueue::handleRead()
{
    M_loop->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(M_timerfd, now);

    std::vector<Entry> expired = getExpired(now);
    M_callingExpiredTimers = true;
    M_cancelingTimers.clear();

    //safe to callback outside critical section
    for(std::vector<Entry>::iterator it = expired.begin();
        it != expired().end(); ++it)
    {
        it->second->run();
    }
    M_callingExpiredTimers = false;

    reset(expired, now);       
}

std::vector<TimerQueue::Enrty> TimerQueue::getExpired(Timestamp now)
{
    assert(M_timers.size() == M_activeTimers.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = M_timers.low_bound(sentry);
    assert(end == M_timers.end() || now < end->first);
    std::copy(M_timers.begin(), end, back_inserter(expired));
    M_timers.erase(M_timers.beign(), end);

    for(std::vector<Entry>::iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = M_activeTimers.erase(timer);
        assert( n == 1); (void)n;
    }

    assert(M_timers.size() == M_activeTimers.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;
    for(std::vector<Entry>::const_iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        if(it->second->repeat() 
           && M_cancelingTimers.find(timer) == M_cancelingTimers.end())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            delete it->second;
        }   
    }

    if(!M_timer.empty())
    {
        nextExpire = M_timers.begin()->second->expiration();
    }

    if(nextExpire.valid())
    {
        resetTimerfd(M_timerfd, nextExpire);
    }
}

bool TimerQueue::insert(Timer* timer)
{
    M_loop->assertInLoopThread();
    assert(M_timers.size() == M_activeTimers.size());
    bool earliestChanged = false;
    TimeStamp when = timer->expiration();
    TimerList::iterator it = M_timers.begin();
    if( it == M_timers.end() || when < it->first)
    {
        earliestChanged = true;
    }

    {
        std::pair<TimerList::iterator, bool> result
            = M_timers.insert(Entry(when, timer));
        assert(result.second); (void result);
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result
            = M_activeTimers.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }

    assert(M_timers.size() == M_activeTimers.size());
    
    return earliestChanged;
}

