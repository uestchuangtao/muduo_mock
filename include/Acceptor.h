#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

///
///Acceptor of incoming TCP connections. 
///

class Acceptor:: boost::noncopyable
{
public:
    typedef boost::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        M_newConnectionCallback = cb;
    }

    bool listenning() const 
    {
        return M_listenning;
    }

    void listen();

private:
    void handleRead();

    EventLoop* M_loop;
    Socket M_acceptSocket;
    Channel M_acceptChannel;
    NewConnectionCallback M_newConnectionCallback;
    bool M_listenning;
    int M_idleFd;
};


#endif