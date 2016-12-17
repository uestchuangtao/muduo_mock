#ifndef TIMERID_H
#define TIMERID_H

class Timer;

///
/// An opaque identifier,for canceling Timer.  
///

class TimerId 
{
public:
    TimerId()
      : M_timer(NULL),
        M_sequence(0)
    {

    }

    TimerId(Timer* timer, int64_t seq)
      : M_timer(timer),
        M_sequence(seq)
    {
        
    }

    friend class TimerQueue;

private:
    Timer* M_timer;
    int64_t M_sequence;    
        
};

#endif