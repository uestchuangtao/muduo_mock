#include "SocketsOps.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>    // snprintf
#include <strings.h> // bzero
namespace
{
    typedef struct sockaddr SA;

    void setNonBlockAndCloseOnExec(int sockfd)
    {
        int flags = ::fcntl(sockfd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        int ret = ::fcntl(sockfd, F_SETFL, flags)

        flags = ::fcntl(sockfd, F_GETFD, 0);
        flags |= FD_CLOEXEC;
        int ret = ::fcntl(sockfd, F_SETFD, flags);
    }
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr)
{
    return static_cast<const struct sockaddr*>((const void *)addr);

struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr)
{
    return static_cast<struct sockaddr*>((const void *)addr);
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr)
{
    return static_cast<const struct sockaddr*>((const void *)addr);
}

const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr)
{
    return static_cast<const struct sockaddr_in*>((const void*)addr);
}

const struct sockaddr_in6* sockets::sockaddr_in6_cast(const struct sockaddr* addr)
{
    return static_cast<const struct sockaddr_in6*>((const void*)addr);
}


int createNonblockingOrDie(sa_family_t family)
{
    int sockfd = socket(family, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0)
    {
        //TODO:
    }
    setNonBlockAndCloseOnExec(sockfd);
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr* addr)
{
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    if(ret < 0)
    {
        //TODO:
    }
}

void  sockets::listenOrDie(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if( ret < 0)
    {
        //TODO:
    }
}

int sockets::accept(int sockfd, const struct sockaddr* addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));
    int connfd = :accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);

    if(connfd < 0)
    {
        int savedErrno = errno;
        switch(savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTUSPP:
                {
                    //TODO:
                    break;
                }
            default:
                //TODO:
                break;

        }
    }
    return connfd;

}

int sockets::connect(int sockfd, const struct sockaddr* sock)
{
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void *buf, size_t count)
{
    return ::read(sockfd, buf, count);
}

size_t sockets::readv(int sockfd, struct iovec *iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}

size_t sockets::write(int sockfd, void *buf, int count)
{
    return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd)
{
    if(::close(sockfd) < 0)
    {
        //TODO:
    }
}

void sockets::shutdownWrite(int sockfd)
{
    if(::shutdown(sockfd, SHUT_WR) < 0)
    {
        //TODO:
    }
}

void sockets::toIpPort(char *buf, size_t size, const struct sockaddr* addr)
{
    toIp(buf, size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    uint16_t port = ntohs(addr4->sin_port);
    assert(size > end);
    snprintf(buf+end, size-end, ":%u", port);
}

void sockets::toIp(char *buf, size_t size, const struct sockaddr* addr)
{
    if(addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if(addr->sa_family == AF_INET6)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

void sockets::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in* addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if(::inet_pton(AF_INET,ip, &addr->sin_addr) <= 0)
    {
        //TODO:
    }


void sockets::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6* addr)
{
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    if(::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
    {
        //TODO:

    }
}

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

struct sockaddr_in6 sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localAddr;
    bzero(&localAddr, sizeof(localAddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(localAddr));
    if(::getsockname(sockfd, sockaddr_cast(&localAddr), &addrlen) < 0)
    {
        //TODO:
    }
    return localAddr;
}

struct sockaddr_in6 sockets::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peerAddr;
    bzero(&peerAddr, sizeof(peerAddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(peerAddr));
    if(::getpeername(sockfd, sockaddr_cast<&peerAddr>, &addlen) < 0)
    {
        //TODO:
    }
    return peerAddr;
}






