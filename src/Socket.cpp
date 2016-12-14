#include "Socket.h"

#include "InetAddress.h"
#include "SocketsOps.h"

#include <strings.h>  //bzero

Socket::~Socket()
{
   sockets::close(M_sockfd);
}

void Socket::bindAddress(const InetAddress& addr)
{
    sockets::bindOrDie(M_sockfd, addr.getSockAddr());
}

void Socket::listen()
{
    sockets::listenOrDie(M_sockfd);
}

int Socket::accept(InetAddress* peeraddr)
{
    struct sockaddr_in6 addr;
    bzero(&addr, sizeof(addr));
    int connfd = sockets::accept(M_sockfd, &addr);
    if(connfd >= 0)
    {
        peeraddr->setSockAddrInet6(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    sockets::shutdownWrite(M_sockfd);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(M_sockfd, IPPROTO_TCP, TCP_NODELAY,
                &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(M_sockfd, SOL_SOCKET, SO_REUSEADDR,
                &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReusePort(bool on)
{
#ifdef SO_RESUEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(M_sockfd, SOL_SOCKET, SO_REUSEPORT,
                            &optval, static_cast<socklen_t>(sizeof(optval)));

     if(ret < 0 && on)
     {
        //TODO:
     }
#else
      if(on)
      {
        //TODO:
      }
#endif // SO_RESUEPORT
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(M_sockfd, SOL_SOCKET, SO_KEEPALIVE,
                &optval， static_cast<socklen_t>(sizeof(optval)))；
}






