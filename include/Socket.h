#ifndef SOCKET_H
#define SOCKET_H

#include <boost/noncopyable.hpp>

class InetAddress;

class Socket : boost::noncopyable
{
    public:
        explicit Socket(int sockfd)
            :M_sockfd(sockfd)
        { }

        ~Socket();

        int fd() const
        {
            return M_sockfd;
        }

        void bindAddress(const InetAddress &localaddr);

        void listen();

        int accept(InetAddress* peeraddr);

        void shutdownWrite();

        void setTcpNoDelay(bool on);

        void setReuseAddr(bool on);

        void setReusePort(bool on);

        void setKeepAlive(bool on);

    private:
        const int M_sockfd;
};

#endif // SOCKET_H
