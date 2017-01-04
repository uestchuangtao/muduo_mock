#include "TcpServer.h"

#include "Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <stdio.h> // snprintf

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg,
                     Option option)
    : M_loop(CHECK_NOTNULL(loop)),
      M_ipPort(listenAddr.toIpPort()),
      M_name(nameArg),
      M_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
      M_threadPool(new EventLoopThreadPool(loop, M_name)),
      M_connectionCallback(defaultConnectionCallback),
      M_messageCallback(defaultMessageCallback),
      M_nextConnId(1)
{
    M_acceptor->setNewConnectionCallback(boost::bind(&TcpServer::newConnection, this, _1, _2));
} 

TcpServer::~TcpServer()
{
    M_loop->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << M_name << "] destructing";

    for(ConnectionMap::iterator it(M_connections.begin());
        it != M_connetions.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();
        conn->getLoop()->runInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();
    }
} 

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    M_threadPool->setThreadNum(numThreads);
} 

void TcpServer::start()
{
    if(M_started.getAndSet(1) == 0)
    {
        M_threadPool->start(M_threadInitCallback);

        assert->(!M_acceptor->listening());
        M_loop->runInLoop(boost::bind(&Acceptor::listen, 
            get_pointer(M_acceptor)));
    }
} 

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    M_loop->assertInLoopThread();
    EventLoop* ioLoop = M_threadPool->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d",M_ipPort.c_str(), M_nextConnId);
    ++M_nextConnId;
    string connName = M_name + buf;

    LOG_INFO << "TcpServer::newConnection [" << M_name
             << "] - new connection [" << connName
             << " from " << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
   
    TcpConnectionPtr conn(new TcpConnection(ioLoop, 
                                            connName, 
                                            sockfd, 
                                            localAddr, 
                                            peerAddr));
     M_connections[connName] = conn;
     conn->setConnectionCallback(M_connectionCallback);
     conn->setMessageCallback(M_messageCallback);
     conn->setWriteCompleteCallback(M_writeCompleteCallback);
     conn->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1));
     ipLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));                                       
} 

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    M_loop->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    M_loop->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << M_name
             << "] - connetion " << conn->name();
    size_t n = M_connections.erase(conn->name());
    (void)n;
    assert(n == 1);
    EventLoop* ioLoop = conn->getLoop;
    ioLoop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}                      