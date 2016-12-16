#ifndef POLLPOLLER_H
#define POLLPOLLER_H

#include "Poller.h"

#include <vector>

struct pollfd;

///
/// IO Multiplexing with poll(2). 
///
class PollPoller : public Poller 
{
public：
    PollPoller(EventLoop *loop);
    virtual ~PollPoller();

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
    virtual void updateChannel(Channel* channel);
    virtual void removeChannel(Channel* channel);

private:
    void fillActiveChannels(int numEvents, 
                            ChannnelList* activeChannels)  const;

    typedef std::vector<struct pollfd> PollFdList;
    PollFdList M_pollfds;  
};


#endif