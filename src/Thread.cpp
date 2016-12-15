#include "Thread.h"
#include "CurrentThread.h"

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/weak_ptr.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread int t_tidStringLength = 6;
    __thread const char* t_threadName = "unknown";
    const bool sameType = boost::is_same<int, pid_t>::value;
    BOOST_STATIC_ASSERT(sameType);
}

namespace detail
{
   pid_t gettid()
    {   
        return static_cast<pid_t>(::syscall(SYS_gettid()));
    }

    void afterFork()
    {
        muduo::CurrentThread::t_cachedTid = 0;
        muduo::CurrentThread::t_threadName = "main";
        CurrentTHread::tid();
    }

    class ThreadNameInitializer
    {
        public:
            ThreadNameInitializer()
            {
                CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                pthread_atfork(NULL,NULL,&afterFork);
            }
    };

    ThreadNameInitializer init;

    struct ThreadData
    {
        typedef Thread::ThreadFunc ThreadFunc;
        ThreadFunc M_func;
        string M_name;
        boost::weak_ptr<pid_t> M_wkTid;

        ThreadData(const ThreadFunc& func,
                   const string& name,
                   const boost::shared_ptr<pid_t>& tid)
                   :M_func(func),
                    M_name(name),
                    M_wkTid(tid)
        { }

        void runInThread()
        {
            pid_t tid = CurrentThread::tid();

            boost::shared_ptr<pid_t> ptid = M_wkTid.lock();
            if(ptid)  /// ???
            {
                *ptid = tid;
                ptid.reset(); 
            }

            CurrentThread::t_threadName = name.empty() ? "muduoTHread" : name.c_str();
            ::prctl(PR_SET_NAME, CurrentThread::t_threadName);
            try
            {
                M_func();
                CurrentThread::t_threadName = "finished";
            }
            catch(const Exception& ex)
            {
                CurrentThread::t_threadName = "crashed";
                fprintf(stderr,"exception caught in Thread %s\n", name.c_ctr());
                fprintf(stderr,"reason: %s\n",ex.what());
                fprintf(stderr,"stack trace: %s\n",ex.stackTrace());
                abort();
            }
            catch(const std::exception& ex)
            {
                CurrentThread::t_threadName = "crashed";
                fprintf(stderr,"exception caught in Thread %s\n", name.c_ctr());
                fprintf(stderr,"reason: %s\n",ex.what());
                abort();
            }
            catch(...)
            {
                CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "unknown exception caught in Thread %s\n", name.c_str());
                throw;
            }
        }
    };

    void* startThread(void *obj)
    {
        ThreadData* data = static_cast<ThreadData*>(obj);
        data->runInThread();
        delete data;
        return NULL;
    }
 
}

void CurrentThread::cacheTid()
{
    if(t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
    }
}

bool CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
    struct timespec ts = { 0, 0 };
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::KMicroSecondsPerSecond);
    ts.tv.nsec = static_cast<long>(usec % TimeStamp::KMicroSecondPerSecond * 1000);
    ::nanosleep(&ts, NULL);
}


AtomicInt32 Thread::M_numCreated;

Thread::Thread(const ThreadFunc& func, const string& n)
    : M_started(false),
      M_joined(false),
      M_pthreadId(0),
      M_tid(new pid_t(0)),
      M_func(func),
      M_name(n)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(M_started && !M_joined)
    {
        pthread_detach(M_pthreadId);
    }
}

void Thread::setDefaultName()
{
    int num = M_numCreated.incrementAndGet();
    if( M_name.empty())
    {
        char buf[32];
        snprintf(buf,sizeof(buf),"Thread%d", num);
        M_name = buf;
    }
}


void Thread::start()
{
    assert(!M_started);
    M_start = true;
    detail::ThreadData* data = new detail::ThreadData(M_func, M_name, M_tid);
    if(pthread_create(&M_pthreadId,NULL, &details::startThread, data))
    {
        M_started = false;
        delete data;
        LOG_SYSFATAL << "Failed in pthread_create";
    }
}

int Thread::join()
{
    assert(M_started);
    assert(!M_joined);
    M_joined = true;
    return pthread_join(M_pthreadId,NULL);
}