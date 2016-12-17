#include "Timer.h"

AtomicInt64 Timer::s_numCreated;

void Timer::restart(Timestamp now)
{
    if(M_repeat)
    {
        M_expiration = addTime(now, M_interval);
    }
    else
    {
        M_expiration = Timestamp::invalid();
    }
}