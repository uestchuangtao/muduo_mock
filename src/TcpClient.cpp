#include "TcpConnection.h"

#include "Logging.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <stdio.h> //snprintf

namespace detail
{
    void removeConnection(EventLoop* loop, const TcpConnection& conn)
    {
       loop->runInLoop(boost::bind(&TcpConnetion::connectDestroyed, conn));
    }

    void removeConnector(const ConnectorPtr& connector)
    {

    }
}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const string& nameArg)
    : M_loop(CHECK_NOTNULL(loop)),
      M_connector(new Connector(loop, serverAddr)),
      M_name(nameArg),
      M_connectionCallback(defaultConnectionCallback),
      M_messageCallback(defaultMessageCallback),                     
      M_retry(false),
      M_connect(true),
      M_nextConnId(1)                    
{
    M_connector->setNewConnectionCallback(boost::bind(&TcpClient::newConnection, this, _1));
    LOG_INFO << "TcpClient::TcpClient[" << M_name << "] - connector " << get_pointer(M_connector);
}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << M_name 
             << "] - connector " << get_pointer(M_connector);
    TcpConnectionPtr conn;
    bool unique = false;
    {
        MutexLockGuard lock(M_mutex);
        unique = M_connetion.unique();
        conn = M_connection;
    }

    if(conn)
    {
        assert(M_loop == conn->getLoop());
        //FIXME not 100% safe, if we are in different thread 
        CloseCallback cb = boost::bind(&detail::removeConnection, M_loop, _1);
        M_loop->runInLoop(boost::bind(&TcpConnection::setCloseCallback, conn, cb));
        if(unique)
        {
            conn->forceClose();
        }
    }
    else
    {
        M_connector->stop();
        // FIXME: HACK
        M_loop->runAfter(1, boost::bind(&details::removeConnector, M_connector));
    }
}

void TcpClient::connect()
{
    LOG_INFO << " TcpClient::connect[" << M_name << "] - connecting to "
             << M_connector->serverAddress().toIpPort();
    M_connect = true;
    M_connector->start();         
}

void TcpClient::disconnect()
{
    M_connect = false;

    {
        MutexLockGuard lock(M_mutex);
        if(M_connection)
        {
            M_connection->shutdown();
        }
    }
}

void TcpClient::stop()
{
    M_connect = false;
    M_connector->stop();
}

void TcpClient::newConnection(int sockfd)
{
    M_loop->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort.c_str(), M_nextConnId);
    ++M_nextConnId;
    string connName = M_name + buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(M_loop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

     conn->setConnectionCallback(M_connectionCallback);  
     conn->setMessageCallback(M_messageCallback);
     conn->setWriteCompleteCallback(M_writeCompleteCallback);
     conn->setCloseCallback(
         boost::bind(&TcpClient:removeConnection, this, _1))；
     {
         MutexLockGuard lock(M_mutex);
         M_connection = conn;
     } 
     conn->connectEstablished();                                   
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    M_loop->assertInLoopThread();
    assert(M_loop == conn->getLoop());

    {
        MutexLockGuard lock(M_mutex);
        assert(M_connection == conn);
        M_connection.reset();
    }

    M_loop->queueInLoop(boost::bind(&TcpConnetion::connectDestroyed, conn));
    if(M_retry && M_connect)
    {
        LOG_INFO << "TcpClient::connect[" << M_name << "] - Reconnecting to "
                 << M_connector->serverAddress().toIpPort();
        M_connector->restart();         
    }
}