#include "Channel.h"
#include "Poller.h"

Poller::Poller(EventLop* loop)
    : M_ownerLoop(loop)
{    
}

Poller::~Poller()
{    
}

bool Poller::hasChannel(Channel* channel) const 
{
    assertInLoopThread();
    ChannelMap::const_iterator it = M_channels.find(channel->fd());
    return it != M_channels.end() && it->second == channel;
}