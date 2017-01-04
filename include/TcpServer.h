#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Atomic.h"
#include "Types.h"
#include "TcpConnection.h"

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details. 

class TcpServer : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const string& nameArg,
              Option option = kNoReusePort);

    ~TcpServer(); //force out-line dtor, for scoped_ptr members. 

    const string& ipPort() const 
    {
        return M_ipPort;
    }

    const string& name() const 
    {
        return M_name;
    }

    EventLoop* getLoop() const 
    {
        return M_loop;
    }

    /// Set the number of threads for handling input.
    ///
    /// Always accepts new connection in loop's thread.
    /// Must be called before @c start
    /// @param numThreads
    /// - 0 means all I/O in loop's thread, no thread will created.
    ///   this is the default value.
    /// - 1 means all I/O in another thread.
    /// - N means a thread pool with N threads, new connections
    ///   are assigned on a round-robin basis.
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb)
    {
        M_threadInitCallback = cb;
    }
    ///valid after calling start()
    boost::shared_ptr<EventLoopThreadPool> threadPool()
    {
        return M_threadPool;
    }

    ///Starts the server if it's not listenning. 
    ///
    ///It's harmless to call it multiple times. 
    ///Thread safe. 
    void start();

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

    /// Set write compelte callback.
    /// Not thread safe. 
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        M_writeCompleteCallback = cb;
    }

  private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd, const InetAddress& peerAddr);
    /// Thread safe. 
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::map<string, TcpConnectionPtr> ConnectionMap;

    EventLoop* M_loop;  //the acceptor loop 
    const string M_ipPort;
    const string M_name;
    boost::scoped_ptr<Acceptor> M_acceptor;
    boost::shared_ptr<EventLoopThreadPool> M_threadPool;
    ConnectionCallback M_connectionCallback;
    MessageCallback M_messageCallback;
    WriteCompleteCallback M_writeCompleteCallback;
    ThreadInitCallback M_threadInitCallback;
    AtomicInt32 M_started;
    //  always in loop thread 
    int M_nextConnId;
    ConnectionMap M_connections;




};

#endif
