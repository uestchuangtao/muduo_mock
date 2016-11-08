#include "SocketsOps.h"

#include <fcntl.h>
#include <errno.h>

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

