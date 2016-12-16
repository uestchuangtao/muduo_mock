#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H

#include "Poller.h"

#include <vector>

struct epoll_event; 


///
/// IO Multiplexing with epoll 
///

class EPollPoller ： public Poller
{
public:
    EPollPoller(EventLoop* loop);
    virtual ~EPollPoller();

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
    virtual void updateChannel(Channel* channel);
    virtual void removeChannel(Channel* channel);

private:
    static const int kInitEventListSize = 16;

    static const char* operationToString(int op);

    void fillActiveChannels(int numEvents,
                            ChannelList* activeChannels) const 
    void update(int operation, Channel* channel);

    typedef std::vector<struct epoll_event> EventList;

    int M_epollfd;
    EventList M_events;                        

};

#endif