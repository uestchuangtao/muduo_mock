#ifndef SOCKET_H
#define SOCKET_H

#include <boost/noncopyable.hpp>

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


    private:
        const int M_sockfd;
};

#endif // SOCKET_H
