#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <assert.h>
#include <boost/noncopyable.hpp>

class MutexLock : boost::noncopyable
{
    public:
        MutexLock()
        {
            pthread_mutex_init(&M_mutex, NULL);
        }

        ~MutexLock()
        {
            pthread_mutex_destroy(&M_mutex);
        }

        void lock()
        {
            pthread_mutex_lock(&M_mutex);
        }

        void unlock()
        {
            pthread_mutex_unlock(&M_mutex);
        }

        pthread_mutex_t* getPthreadMutex()
        {
            return &M_mutex;
        }

    private:
        pthread_mutex_t M_mutex;
};

class MutexLockGuard : boost::noncopyable
{

    public:
        explicit MutexLockGuard(MutexLock& mutex)
            :M_mutex(mutex)
        {
            M_mutex.lock();
        }

        ~MutexLockGuard()
        {
            M_mutex.unlock();
        }

    private:
        MutexLock& M_mutex;
};

#define MutexLockGuard(x) error "Missing guard object name"



#endif // MUTEX_H
