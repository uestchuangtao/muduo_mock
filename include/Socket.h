#ifndef SOCKET_H
#define SOCKET_H

#include <boost/noncopyable.hpp>

class Socket : boost::noncopyable
{
    public:
        explicit Socket(int sockfd)
        :mSockfd(sockfd)
        { }
        ~Socket();

        int fd() const
        {
            return mSockfd;
        }


    private:
        const int mSockfd;
};

#endif // SOCKET_H
