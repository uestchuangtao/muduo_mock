#include "Acceptor.h"

#include "Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAdddr, bool reuseport)
    : M_loop(loop),
      M_acceptSocket(sockets::createNonblockingOrDie(listenAddr.family())),
      M_acceptChannel(loop, M_acceptSocket.fd()),
      M_listenning(false),
      M_idleFd(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(M_idleFd >= 0);
    M_acceptSocket.setReuseAddr(true);
    M_acceptSocket.setReusePort(reuseport);
    M_acceptSocket.bindAddress(listenAddr);
    M_acceptChannel.setReadCallback(
        boost::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    M_acceptChannel.disableAll();
    M_acceptChannel.remove();
    ::close(M_idleFd);
}  

void Acceptor::listen()
{
    M_loop->assertInLoopThread();
    M_listenning = true;
    M_acceptSocket.listen();
    M_acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
    M_loop->assertInLoopThread();
    InetAddrss peerAddr;
    //FIXME loop until no more 
    int connfd = M_acceptSocket.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(M_newConnectionCallback)
        {
            newConnectionCallback(connfd, peerAddr);
        }
        else
        {
            sockets::close(connfd);
        }
    }
    else
    {
        LOG_SYSERR << "in Acceptor::handleRead";
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        if (errno == EMFILE)
        {
            ::close(M_idleFd);
            M_idleFd = ::accept(M_acceptSocket.fd(), NULL, NULL);
            ::close(M_idleFd);
            M_idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}    