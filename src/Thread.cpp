#include "Thread.h"

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
    __thread const char * t_threadName = "unknown";
    const bool sameType = boost::is_same<int, pid_t>::value;
    BOOST_STATIC_ASSERT(sameType);
}

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