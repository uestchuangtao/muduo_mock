#ifndef PLLOER_H
#define PLLOER_H

#include <map> 
#include <vector>
#include <boost/noncopyable.hpp>
#include "Timestamp.h"
#include "EventLoop.h"

class Channel;


///
///Base class for IO Multiplexing
///
///This class doesn‘t own the Channel objects
class Poller : boost::noncopyable 
{
public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop);
    virtual ~Poller();

    /// Poller the I/0 events. 
    /// Must be called in the loop thread. 
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    ///Changes the interested I/O events.  
    ///Must be called in the loop thread.  
    virtual void updateChannel(Channel* channel) = 0;

    ///Remove the channel, when it destructs. 
    ///Must be called in the loop thread. 
    virtual void removeChannel(Channel* channel) = 0;

    virtual bool hasChannel(Channel* channel) const;

    static Poller* newDefaultPoller(EventLoop* loop);

    void assertInLoopThread() const 
    {
        M_ownerLoop->assertInLoopThread();
    }

protected:
    typedef std::map<int, Channel*> ChannelMap;
    ChannelMap M_channels;

private:
    EventLoop* M_ownerLoop;
};

#endif
