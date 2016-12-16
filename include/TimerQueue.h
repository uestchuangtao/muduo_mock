#ifndef TIMEQUEUE_H
#define TIMEQUEUE_H

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include "Mutex.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue
/// No guarantee that the callback will be on time. 
///

class TimerQueue : boost::noncopyable
{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    ///
    /// Schedules the callback to be run at given time,
    /// repeats if @c interval > 0.0. 
    ///
    /// Must be thread safe. Usually be called from other threads.  
    TimerId addTimer(const TimerCallback& cb,
                     Timerstamp when,
                     double interval);

    void cancel(TimerId timerId);  

private:

    // FIXME : use unique_ptr<Timer> instead of raw pointers. 
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer * timer);
    void cancelInLoop(TimerId timerId);
    // called when timerfd alarms
    void handleRead();
    // move out all expired timers 
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* M_loop;
    const int M_timerfd;
    Channel M_timerfdChannel;
    // Timer list sorted by expiration
    TimerList M_timers;

    // for cancel()
    ActiveTimerSet M_activeTimers;
    bool M_callingExpiredTimers;  /*atomic*/
    ActiveTimerSet M_cancelingTimers;
};


#endif