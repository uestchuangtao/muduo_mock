#ifndef TIMER_H
#define TIMER_H


#include <boost/noncopyable.hpp>

#include "Atomic.h"
#include "Timestamp.h"
#include "Callbacks.h"

class Timer : boost::noncopyable
{
public:
    Timer(const TimerCallback &cb, Timestamp when, double interval)
        : M_callback(cb),
          M_expiration(when),
          M_interval(interval),
          M_repeat(interval > 0.0)
          M_sequence(s_numCreated.incrementAndGet())
    {   

    }

    void run() const
    {
        M_callback();
    } 

    Timestamp expiration() const 
    {
        return M_expiration;
    }

    bool repeat() const 
    {
        return M_repeat;
    }

    int64_t sequence() const 
    {
        return M_sequence;
    }

    void restart(Timestamp now);

    static int64_t numCreated()
    {
        return s_numCreated.get();
    }


private:
    const TimerCallback M_callback;
    Timestamp M_expiration;
    const double M_interval;
    const bool M_repeat;
    const int64_t M_sequence;
    
    static AtomicInt64 s_numCreated;

};


#endif