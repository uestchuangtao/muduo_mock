#include "PollPoller.h"

#include "Logging.h"
#include "Types.h"
#include "Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>

PollPoller::PollPoller(EventLoop* loop)
    :Poller(loop)
{
}   

PollPoller::~PollPoller()
{
} 

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    // XXX pollfds shouldn't change
    int numEvents = ::poll(&*M_pollfds.begin(), M_pollfds.size(), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
    }
    else if(numEvents == 0)
    {
        LOG_TRACE << "nothing happended";
    }
    else
    {
        if(savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "PollPoller::poll()";
        }
    }
    return now;
}

void PollPoller::fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const
{
    for(PollFdList::const_iterator pfd = M_pollfds.begin();
        pfd != M_pollfds.end() && numEvents > 0; ++pfd)
    {
        if(pfd->revents > 0)
        {
            --numEvents;
            ChannelMap::const_iterator ch = M_channels.find(pfd->fd);
            assert(ch != M_channels.end());
            Channel* channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->set_revents(pfd->revents);
            M_activeChannels.push_back(channel);
        }
    }
} 

void PollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = "<<channel->events();
    if(channel->index() < 0)
    {
        //a new one, add to pollfds
        assert(M_channels.find(channel->fd()) == M_channels.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events())；
        pfd.revents = 0;
        M_pollfds.push_back(pfd);
        int idx = static_cast<int>(M_pollfds.size())-1;
        channel->set_index(idx);
        M_channels[pfd.fd] = channel;
    }
    else 
    {
        //update existing one 
        assert(M_channels.find(channel->fd()) != M_channels.end());
        assert(M_channels[channel->fd()] == channel);
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(M_pollfds.size()));
        struct pollfd& pfd = M_pollfds[idx];
        assert(pfd.fd == channel.fd() || pfd.fd == -channel.fd()-1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if(channel->isNoneEvent())
        {
            pfd.fd = -channel->fd()-1;
        }
    }
}

void PollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << " fd = " << channel->fd();
    assert(M_channels.find(channel->fd()) != M_channels.end());
    assert(M_channels[channel->fd] == channel);
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(M_pollfds.size()));
    const struct pollfd& pfd = M_pollfds[idx]; (void)pfd;
    assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
    size_t n = M_channels.erase(channel->fd());
    assert(n == 1); (void)n;
    if(implicit_cast<size_t>(idx) == M_pollfds.size()-1)
    {
        M_pollfds.pop_back();
    }
    else
    {
        int channelAtEnd = M_pollfds.back().fd;
        iter_swap(M_pollfds.begin()+idx, M_pollfds.end()-1);
        if(channelAtEnd < 0)
        {
            channelAtEnd = -channelAtEnd-1;
        }
        M_channels[channelAtEnd]->set_index(idx);
        M_pollfds.pop_back();
    }
}

