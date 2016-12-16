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

    //for Poller 
    int index()
    {
        return M_index;
    }

    void set_index(int idx)
    {
        M_index = idx;
    }

    //for debug
    string reventsToString() const;
    string eventsToString() const;

    void doNotLogHup()
    {
        M_logHup = false;
    }

    EventLoop* ownerLoop()
    {
        return M_loop;
    }

    void remove;

private:
    static string eventsToString(int fd, int ev);

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* M_loop;
    const int M_fd;
    int M_events;
    int M_revents; // it's the received event types of epoll or poll
    int M_index;
    bool M_logHup;  

    boost::weak_ptr<void> M_tie;
    bool M_tied;
    bool M_eventHandling;
    bool M_addedToLoop;
    ReadEventCallback M_readCallback;
    EventCallback M_writeCallback;
    EventCallback M_closeCallback;
    EventCallback M_errorCallback;   

};

#endif