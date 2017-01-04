#ifndef TCPCLIENT_H
#define TCPCLIETN_H

#include <boost/noncopyable.hpp>

#include "Mutex.h"
#include "TcpConnection.h"

class  Connector;
typedef boost::shared_ptr<Connector> ConnectorPtr;

class TcpClient : boost::noncopyable
{
public:
    TcpClient(EventLoop* loop,
              const InetAddress& serverAddr,
              const string& nameArg);

    ~TcpClient(); //force out-line dtor, for scoped_ptr members 

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const 
    {
        MutexLockGuard lock(M_mutex);
        return M_connection;
    }

    EventLoop* getLoop() const 
    {
        return M_loop;
    }

    bool retry() const 
    {
        return M_retry;
    }

    void enableRetry()
    {
        M_retry = true;
    }

    const string& name() const 
    {
        return M_name;
    }

    /// Set connection callback. 
    /// Not thread safe. 

    void setConnectionCallback(const ConnectionCallback& cb)
    {
        M_connectionCallback = cb;
    }

    /// Set message callback. 
    /// Not thread safe. 
    void setMessageCallback(const MessageCallback& cb)
    {
        M_messageCallback = cb;
    }

    /// Set write complete callback. 
    /// Not thread safe. 
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        M_writeCompleteCallback = cb;
    }

private:
    ///Not thread safe, but in loop 
    void newConnection(int sockfd);
    ///Not thread safe, but in loop 
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* M_loop;
    ConnectorPtr M_connector;
    const string M_name;
    ConnectionCallback M_connectionCallback;
    MessageCallback M_messageCallback;
    WriteCompleteCallback M_writeCompleteCallback;
    bool M_retry; //atomic
    bool M_connect; //atomic
    //always in loop thread 
    int M_nextConnId;
    mutable MutexLock M_mutex;
    TcpConnectionPtr M_connection; // @GuardedBy mutex
};


#endif