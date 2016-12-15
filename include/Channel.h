#ifndef CHANNEL_H
#define CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "Timestamp.h"

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : boost::noncopyable
{
public:
    typedef boost::function<void ()> EventCallback;
    typedef boost::function<void (Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    void setReadCallback(const ReadEventCallback& cb)
    {
        M_readCallback = cb;
    }

    void setWriteCallback(const EventCallback& cb)
    {
        M_writeCallback = cb;
    }

    void setCloseCallback(const EventCallback& cb)
    {
        M_closeCallback = cb;
    }

    void setErrorCallback(const EventCallback& cb)
    {
        M_errorCallback = cb;
    }

    ///tie this channel to the owner object managed by shared_ptr,
    ///prevent the owner object being destroyed in handleEvent.
    void tie(const boost::shared_ptr<void>&);

    int fd() const 
    {
        return M_fd;
    }

    int events() const 
    {
        return M_events;
    }

    void set_revents(int revt)  //used by pollers
    {
        M_revents = revt;
    }

    bool isNoneEvent() const 
    {
        return M_events = kNoneEvent;
    }

    void enableReading()
    {
        M_events |= kReadEvent;
        update();
    }

    void disableReading()
    {
        M_events &= ~kReadEvent;
        update();
    }

    void enableWriting()
    {
        M_events |= kWriteEvent;
        update();
    }

    void disableWriting()
    {
        M_events &= ~kWriteEvent;
        update();
    }

    void disableAll()
    {
        M_events = kNoneEvent;
        update();
    }

    bool isWriting() const
    {
        return M_events & kWriteEvent;
    }

    bool isReading() const 
    {
        return M_events & kReadEvent;
    }

};

#endif