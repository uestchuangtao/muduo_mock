#ifndef CONDITION_H
#define CONDITION_H

#include "Mutex.h"

#include <boost/noncopyable.hpp>
#include <pthread.h>

class Condition : boost::noncopyable
{

    public:
        explicit Condition(MutexLock &mutex)
            :M_mutex(M_mutex)
        {
            pthread_cond_init(&M_pcond);
        }

        ~Condition()
        {
            pthread_cond_destroy(&M_pcond);
        }

        void wait()
        {
            pthread_cond_wait(&M_pcond, M_mutex.getPthreadMutex());
        }

        void notify()
        {
            pthread_cond_signal(&M_pcond);
        }

        void notifyAll()
        {
            pthread_cond_broadcast(&M_pcond);
        }


    private:
        MutexLock & M_mutex;
        pthread_cond_t M_pcond;
}


#endif // CONDITION_H
