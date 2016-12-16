#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"

#include <sstream>

#include <poll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : M_loop(loop),
      M_fd(fd),
      M_events(0),
      M_revents(0),
      M_index(-1),
      M_logHup(true),
      M_tied(false),
      M_eventHandling(false),
      M_addedToLoop(false)
{    
}

Channel::~Channel()
{
    assert(!M_eventHandling);
    assert(!M_addedToLoop);
    if(M_loop->isInLoopThread())
    {
        assert(!M_loop->hasChannel(this));
    }
}

void Channel::tie(const boost::shared_ptr<void>& obj)
{
    M_tie = obj;
    M_tied = true;
}

void Channel::update()
{
    M_addedToLoop = true;
    M_loop->updateChannel(this);
}

void Channel::remove()
{
    assert(isNoneEvent());
    M_addedToLoop = false;
    M_loop->removeChannel(this);
}

void Channel::handleEvent(TimeStamp receiveTime)
{
    boost::shared_ptr<void> guard;
    if(M_tied)
    {
        guard = M_tie.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(TimeStamp receiveTime)
{
    M_eventHandling = true;
    LOG_TRACE << reventsToString();
    if((M_revents & POLLHUP) && !(M_revents & POLLIN))
    {
        if(M_logHup)
        {
            LOG_WARN << "fd = " << M_fd << " Channel::handle_event() POLLHUP";
        }
        if(M_closeCallback)
            M_closeCallback();
    }

    if(M_revents & POLLINVAL)
    {
        LOG_WARN << "fd =" << M_fd << " Channel::handle_event() POLLINVAL";
    }

    if(M_revents & (POLLERR | POLLINVAL))
    {
        if(M_errorCallback)
            M_errorCallback();  
    }
    if(M_revents & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if(M_readCallback)
            M_readCallback(receiveTime);
    }
    if(M_revents & POLLOUT)
    {
        if(M_writeCallback)
            writeCallback();
    }
    M_eventHandling = false; 
}

string Channel::reventsToString() const 
{
    return eventsToString(M_fd, M_revents);
}

string Channel::eventsToString() const 
{
    return eventsToString(M_fd, M_events);
}

string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN)
        oss << "IN ";
    if (ev & POLLPRI)
        oss << "PRI ";
    if (ev & POLLOUT)
        oss << "OUT ";
    if (ev & POLLHUP)
        oss << "HUP ";
    if (ev & POLLRDHUP)
        oss << "RDHUP ";
    if (ev & POLLERR)
        oss << "ERR ";
    if (ev & POLLNVAL)
        oss << "NVAL ";

    return oss.str().c_str();
}
