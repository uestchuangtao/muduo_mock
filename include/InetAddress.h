#ifndef INETADDRESS_H
#define INETADDRESS_H


#include <StringPiece.h>

#include <netinet/in.h>


class InetAddress
{
    public:
        explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
        InetAddress(StringArg &ip, uint16_t port, bool ipv6 = false);

        explicit InetAddress(const struct sockaddr_in& addr)
            :M_addr(addr)
        { }

        explicit InetAddress(const struct sockaddr_in6& addr)
            :M_addr6(addr)
        { }

        sa_family_t family() const
        {
            return M_addr.sin_family;
        }

        string toIp() const;
        string toIpPort() const;
        uint16_t toPort() const;

        const struct sockaddr* getSockAddr() const
        {
            return sockets::sockaddr_cast(&M_addr6);
        }

        void setSockAddrInet6(const struct sockaddr_in6 &addr6)
        {
            M_addr6 = addr6;
        }

        static bool resolve(StringArg hostname, InetAddress*result);


    private:
        union
        {
            struct sockaddr_in M_addr;
            struct sockaddr_in6 M_addr6;
        };
};

#endif // INETADDRESS_H
