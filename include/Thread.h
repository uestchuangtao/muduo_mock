#ifndef THREAD_H
#define THREAD_H

#include <Atomic.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <pthread.h>
#include <string>

class Thread : boost::noncopyable
{
    public:
        typedef boost::function<void ()> ThreadFunc;

        explicit Thread(const ThreadFunc&, const string& name = string());

        ~Thread();

        void start();
        void join(); //return pthread_join()

        bool started() const 
        {
            return M_started;
        }
        
        pid_t tid() const 
        {
            return *M_tid;
        }

        const string& name() const 
        {
            return M_name;
        }

        static int numCreated()
        {
            return M_numCreated.get();
        }

    private:
        void setDefaultName();

        bool M_started;
        bool M_joined;
        pthread_t M_pthreadId;
        boost::shared_ptr<pid_t> M_tid;
        ThreadFunc M_func;
        string M_name;

        static AtomicInt32 M_numCreated;
};

#endif