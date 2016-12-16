#include "EPollPoller.h"
#include "Logging.h"
#include "Channel.h"

#include <boost/static_assert.hpp>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

// On linux,the constants of poll(2) and epoll(4)
// are expected to be the same. 
BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

namespace
{
    const int kNew = -1;
    const int kAdded = 1;
    const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    :Poller(loop),
     M_epollfd(::epoll_create1(EPOLL_CLOEXEC)),
     M_events(kInitEventListSize)
{
    if(M_epollfd < 0)
    {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
}

EPollPoller::~EpollPoller()
{
    ::close(M_epollfd);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_TRACE << "fd total count " << M_channels.size();
    int numEvents = :epoll_wait(M_epollfd,
                                &*M_events.begin(),
                                static_cast<int>(M_events.size()),
                                timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        {
            if(implicit_cast<size_t>(numEvents) == M_events.size())
            {
                M_events.resize(M_events.size()*2);
            }
        }
    }
    else if(numEvents == 0)
    {
        LOG_TRACE << "nothing happended";
    }
    else
    {
        //error happends, log uncommon ones
        if(savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::poll()";
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const 
{
    assert(implicit_cast<size_t>(numEvents) <= M_events.size());
    for(int i = 0; i < numEvents; ++i)
    {
        CHannel *channel = static_cast<Channel*>(M_events[i].data.ptr);
    #ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = M_channels.find(fd);
        assert(it != M_channels.end());
    #endif
        channel->set_revents(M_events[i].events);
        M_activeChannels.push_back(channel);
    }
}

void EPollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    const int index = channel->index();
    LOG_TRACE << "fd = "<<channel->fd()
        << "events = "<<channel->events() << "index = " <<index;
    if(index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if(index == kNew)
        {
            assert(M_channels.find(fd) == M_channels.end());
            M_channels[fd] = channel;
        }
        else // index == kDeleted
        {
            assert(M_channels.find(fd) != M_channels.end());
            assert(M_channels[fd] == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        (void)fd;
        assert(M_channels.find(fd) != M_channels.end());
        assert(index == kAdded);
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " <<fd;
    assert(M_channels.find(fd) != M_channels.end());
    assert(M_channels[fd] == channel); 
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = M_channels.erase(fd);
    (void)n;
    assert(n == 1);

    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
        << " fd = " << fd << "event = { "<<channel->eventsToString() << "}";
    if(::epoll_ctl(M_epollfd, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << "fd =" << fd;
        }
        else 
        {
            LOG_SYSFATAL << "epoll_ctl op ="<< operationToString(operation) << "fd =" << fd;
        }
    }

}

const char* EPollPoller::operationToString(int op)
{
    switch(op)
    {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");  
            return "Unknown Operation";         
    }
}

