#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "StringPiece.h"
#include "Types.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>


//struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

class Channel;
class EventLoop;
class Socket;

class TcpConnection : boost::noncopyable,
                      public boost::enable_shared_from_this<TcpConnection>
{

public:
    /// Constructs a TcpConnection with a connected sockfd
    ///
    /// User should not create this object.  
    TcpConnection(EventLoop* loop,
                  const string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const 
    {
        return M_loop;
    }

    const string& name() const 
    {
        return M_name;
    }

    const InetAddress& localAddress() const 
    {
        return M_localAddr;
    }

    const InetAddress& peerAddress() const 
    {
        return M_peerAddr;
    }

    bool connected() const 
    {
        return M_state == kConnected;
    }

    bool disconnected() const 
    {
        return M_state == kDisconnected;
    }

    //return true if success.  
    bool getTcpInfo(struct tcp_info*) const;
    string getTcpInfoString() const;

    void send(const void* message, int len);
    void send(const StringPiece& message);
    void send(Buffer* message); //this one will swap data 
    void shutdown(); //Not thread safe, no simultaneous calling
    void forceClose();
    void forceCloseWithDelay(double seconds);
    void setTcpNoDelay(bool on);
    void startRead();
    void stopRead();
    bool isReading() const 
    {
        return M_reading;  //NOT thread safe, may race with start/stopReadingInLoop
    }

    void setContext(const boost::any& context)
    {
        M_context = context;
    }

    const boost::any& getContext() const 
    {
        return M_context;
    }

    boost::any* getMutableContext()
    {
        return &M_context;
    }

    void setConnectionCallback(const ConnectionCallback& cb)
    {
        M_connectionCallback = cb;
    }

    void setMessageCallback(const MessageCallback& cb)
    {
        M_messageCallback = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        M_writeCompleteCallback = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        M_highWaterMarkCallback = cb;
        M_highWaterMark = highWaterMark;
    }

    //Advanced interface 
    Buffer* inputBuffer()
    {
        return &inputBuffer;
    }

    Buffer* outputBuffer()
    {
        return &outputBuffer;
    }
    

    ///Internal use only 
    void setCloseCallback(const CloseCallback& cb)
    {
        M_closeCallback = cb;
    }

    //called when TcpServer accepts a new connection 
    void connectEstablished(); //should be called only once
    //called when TcpServer has remove me from its map 
    void connectDestroyed();  //should be called only once


private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
   
    void sendInLoop(const StringPiece& message);
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    void forceCloseInLoop();
    void setState(StateE s)
    {
        M_state = s;
    }
    const char* stateToString() const;
    void startReadInLoop();
    voiid stopReadInLoop();

    EventLoop* M_loop;
    const string M_name;
    StateE M_state;  //FIXME: use atomic variable
    //we don't expose those classes to client

    boost::scoped_ptr<Socket> M_socket;
    boost::scoped_ptr<Channel> M_channel;

    const InetAddress M_localAddr;
    const InetAddress M_peerAddr;
    ConncectionCallback M_connectionCallback;
    MessageCallback M_messageCallback;
    WriteCompleteCallback M_writeCompleteCallback;
    HighWaterMarkCallback M_highWaterMarkCallback;
    CloseCallback M_closeCallback;
    size_t M_highWaterMark;
    Buffer M_inputBuffer;
    Buffer M_outputBuffer;  //FIXME: use list<Buffer> as output buffer
    boost::any M_context;
    bool M_reading;
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;                      

#endif