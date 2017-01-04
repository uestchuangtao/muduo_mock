#include "TcpConnection.h"

#include "Logging.h"
#include "WeakCallback.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <errno.h>

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    //do not call conn->forceClose(), because some users want to register message callback only.           
}

void defaultMessageCallback(const TcpConnectionPtr&,
                            Buffer* buf,
                            Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : M_loop(CHECKOUT_NOTNULL(loop)),
      M_name(nameArg),
      M_state(kConnecting),
      M_socket(new Socket(sockfd)),
      M_channel(new Channel(loop, sockfd)),
      M_localAddr(localAddr),
      M_peerAddr(peerAddr),
      M_highWaterMark(64*1024*1024),
      M_reading(true)
{
    M_channel->setReadCallback(
        boost::bind(&TcpConnection::handleRead, this, _1));
    M_channel->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    M_channel->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    M_channel->setErrorCallback(
        boost::bind(&TcpConnection::handleError,this));
    
    LOG_DEBUG << "TcpConnection::ctor[" << M_name << "] at " << this 
              << "fd=" << sockfd;
    M_socket->setKeepAlive(true);            
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << M_name << "] at " << this 
              << " fd=" << M_channel->fd()
              << " state="<< stateToString();
    assert(M_state == kDisconnected);          
}      


bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const 
{
    return M_socket->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const 
{
    char buf[1024] = {0};
    M_socket->getTcpInfoString(buf, sizeof(buf));
    return buf;
}

void TcpConnection::send(const void* data, int len)
{
    send(StringPiece(static_cast<const char*>(data),len));
}

void TcpConnection::send(const StringPiece& message)
{
    if(M_state == kConnected)
    {
      if(M_loop->isInloopThread())
      {
          sendInLoop(message);
      }
      else 
      {
          M_loop->runInLoop(boost::bind(&TcpConnection::sendInLoop, 
                                        this, 
                                        message.as_string()));
      }  
    }
}

void TcpConnection::send(Buffer* buf)
{
    if(M_state == kConnected)
    {
        if(M_loop->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else 
        {
            M_loop->runInLoop(boost::bind(&TcpConnection::sendInLoop, 
                                          this, 
                                          buf->retrieveAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    M_loop->assertInLoopThread();
    sszize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if(M_state == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    //if no thing in output queue,try writing directly
    if(!M_channel->isWriting() && M_outputBuffer.readableBytes() == 0)
    {
        nwrote = sockets::write(M_channel->fd(), data, len);
        if(nwrote >= 0)
        {
            remaining = len - nwrote;
            if(remaining == 0 && M_writeCompleteCallback)
            {
                M_loop->queueInLoop(boost::bind(M_writeCompleteCallback, shared_from_this()));
            }
        }
        else  //nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if(errno == EPIPE || errno == ECONNRESET) //FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    if(!faultError && remaining > 0)
    {
        size_t oldLen = M_outputBuffer.readableBytes();
        if(oldLen + remaining >= M_highWaterMark
           && oldLen < M_highWaterMark
           && M_highWaterMarkCallback)
        {
            M_loop->queueInLoop(boost::bind(M_highWaterMarkCallback, shared_from_this(), oldLen + remaining));
        }

        M_outputBuffer.append(static_cast<const char*>(data)+nwrote, remaining);
        if(!M_channel->isWriting())
        {
            M_channel->enableWriting();
        }   
    }
}

void TcpConnection::shutdown()
{
    if(M_state == kConnected)
    {
        setstate(kDisconnecting);
        M_loop->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    M_loop->assertInLoopThread();
    if(!M_channel->isWriting())
    {
        M_socket->shutdownWrite();
    }
}

void TcpConnection::forceClose()
{
    //FIXME: use compare and swap
    if(M_state == kConnected || M_state = kDisconnecting)
    {
        setState(kDisconnecting);
        M_loop->queueInLoop(boost::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    if(M_state == kConnected || M_state == kDisconnecting)
    {
       setState(kDisconnecting);
       M_loop->runAfter(seconds, 
                        makeWeakCallback(shared_from_this(),
                                         &TcpConnection::forceClose));
    }
}

void TcpConnection::forceCloseInLoop()
{
    M_loop->assertInLoopThread();
    if(M_state == kConnected || M_state == kDisconnecting)
    {
        //as if we received 0 byte in handleRead();
        handleClose();
    }
}

const char* TcpConnection::stateToString() const 
{
    switch(M_state)
    {
        case kDisconnected:
            return "kDisconnted";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";

    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    M_socket->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
    M_loop->runInLoop(boost::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
    M_loop->assertInLoopThread();
    if(!M_reading || !M_channel->isReading())
    {
        M_channel->enableReading();
        M_reading = true;
    }
}

void TcpConnection::stopRead()
{
    M_loop->runInLoop(boost::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
    M_loop->assertInLoopThread();
    if(M_reading || M_channel->isReading())
    {
        M_channel->disableReading();
        M_reading = false;
    }
}

void TcpConnection::connectEstablished()
{
    M_loop->assertInLoopThread();
    assert(M_state == kConnecting);
    setState(kConnected);
    M_channel->tie(shared_from_this());
    M_channel->enableReading();

    M_connectionCallback(shared_from_this())；
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    M_loop->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = M_inputBuffer.readFd(M_channel->fd(), &savedErrno);
    if(n > 0)
    {
        M_messageCallback(shared_from_this(), &M_inputBuffer, receiveTime);
    }
    else if( n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    M_loop->assertInLoopThread();
    if(M_channel->isWriting())
    {
        ssize_t n = sockets::write(M_channel->fd(),
                                   M_outputBuffer.peek(),
                                   M_outputBuffer.readableBytes());
        if(n > 0)
        {
            M_outputBuffer.retrieveAll();
            if(M_outputBuffer.readableBytes() == 0)
            {
                M_channel->disableWriting();
                if(M_writeCompleteCallback)
                {
                    M_loop->queueInLoop(boost::bind(M_writeCompleteCallback, shared_from_this));
                }
                if(M_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        } 
        else
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }                          
    }
    else
    {
        LOG_TRACE << "Connection fd = " << M_channel->fd()
                  << "is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    M_loop->assertInLoopThread();
    LOG_TRACE << "fd = " << M_channel->fd() << " state = " << stateToString();
    assert(M_state == kConnected || M_state == kDisconnecting);
    //we don't close fd, leave it to dtor, so we can find leaks easily. 
    setState(kDisconnected);
    M_channel->disableAll();
    TcpConnectionPtr guardThis(shared_from_this());
    M_connectionCallback(guardThis);
    //must be the last line
    M_closeCallback(guardThis);
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(M_channel->fd());
    LOG_ERROR << "TcpConnection::handleError [" << M_name
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}









