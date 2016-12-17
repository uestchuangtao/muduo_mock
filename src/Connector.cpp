#include "Connector.h"

#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <errno.h>

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : M_loop(loop),
      M_serverAddr(serverAddr),
      M_connect(false),
      M_state(kDisconnected),
      M_retryDelayMs(kInitRetryDelayMs)
{
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "dtor[" << this << "]";
    assert(!M_channel);
}

void Connector::start()
{
    M_connect = true;
    M_loop->runInLoop(boost::bind(&Connector::startInLoop, this)); //FIXME:  unsafe
}

void Connector::startInLoop()
{
    M_loop->assertInLoopThread();
    assert(M_state == kDisconnected);
    if(M_connect)
    {
        connect();
    }
    else 
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stop()
{
    M_connect = false;
    M_loop->queueInLoop(boost::bind(&Connector::stopInLoop, this)); //FIXME: unsafe 
    //FIXME: cancel timer
}

void Connector::stopInLoop()
{
    M_loop->assertInLoopThread();
    if(M_state == kConnecting)
    {
        setState(kDisconneted);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNonblockingOrDie(M_serverAddr.family());
    int ret = sockets::connect(sockfd, M_serverAddr.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch(savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

       case EAGAIN:
       case EADDRINUSE:
       case EADDRNOTAVAIL:
       case ECONNREFUSED:
       case ENETUNREACH:
            retry(sockfd);
            break;
        
        case EACCESS:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;

        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;

    }
}

void Connector::restart()
{
    M_loop->assertInLoopThread();
    setState(kDisconnected);
    M_retryDelayMs = kInitRetryDelayMs;
    M_connected = true;
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!M_channel);
    M_channel.reset(new Channel(M_loop, sockfd));
    M_channel->setWriteCallback(
        boost::bind(&Connector::handleWrite, this)); //FIXME : unsafe
    M_channel->setErrorCallback(
        boost::bind(&Connector::handleError, this)); //FIXME: unsafe
   
   M_channel->enableWriting();
}

int Connector::removeAndResetChannel()
{
    M_channel->disableAll();
    M_channel->remove();
    int sockfd = M_channel->fd();
    // Can't reset channel here, because we are inside Channel::handleEvent  
    M_loop->queueInLoop(boost::bind(&Connector::resetChannel,this));  //FIXME : unsafe
    return sockfd;
}

void Connector::resetChannel()
{
    M_channel.reset();
}

void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite " << M_state;

    if(M_state == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if(err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror_tl(err);
            retry(sockfd);
        }
        else if(sockets::isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if(M_connect)
            {
                M_newConnectionCallback(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else 
    {
        //what happended?
        assert(M_state == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state=" << M_state;
    if(M_state == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if(M_connect)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << M_serverAddr.toIpPort()
                 << " in " << M_retryDelayMs << " milliseconds. ";
        M_loop->runAfter(M_retryDelayMs/1000.0,
                         boost::bind(&Connector::startInLoop, shared_from_this()));
        M_retryDelayMs = std::min(M_retryDelayMs * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}





