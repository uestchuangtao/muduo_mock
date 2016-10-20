#include "InetAddress.h"

#include <netinet/in.h>
#include <netdb.h>
#include <strings.h> //bzero

#include <boost/static_assert.hpp>

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t KInaddrLoopback = INADDR_LOOPBACK;

BOOST_STATIC_ASSERT(sizeof(InetAddress) == sizeof(struct sockaddr_in6));
BOOST_STATIC_ASSERT(offsetof(sockaddr_in, sin_family) == 0);
BOOST_STATIC_ASSERT(offsetof(sockaddr_in6, sin6_family) == 0);
BOOST_STATIC_ASSERT(offsetof(sockaddr_in, sin_port) == 2);
BOOST_STATIC_ASSERT(offsetof(sockaddr_in6, sin6_port) == 2);

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    BOOST_STATIC_ASSERT(offsetof(InetAddress, M_addr) == 0);
    BOOST_STATIC_ASSERT(offsetof(InetAddress, M_addr6) == 0);
    if(ipv6)
    {
        bzero(&M_addr6, sizeof(M_addr6));
        M_addr6.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        M_addr6.sin6_addr = ip;
        M_addr6.sin6_port = htons(port);
    }
    else
    {
        bzero(&M_addr, sizeof(M_addr));
        M_addr.sin_family = AF_INET;
        in_addr ip = loopbackOnly ? KInaddrLoopback : kInaddrAny;
        M_addr.sin_addr.s_addr = htonl(ip);
        M_addr.sin_port = htons(port);
    }

}

InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6)
{
    if(ipv6)
    {
        bzero(&M_addr6, sizeof(M_addr6));
        sockets::fromIpPort(ip.c_str(), port, &M_addr6);
    }
    else
    {
        bzero(&M_addr, sizeof(M_addr));
        sockets::fromIpPort(ip.c_str(, port, &M_addr));
    }

}

string InetAddress::toIpPort() const
{
    char buf[64] = "";
    sockets::toIpPort(buf, sizeof(buf), getSockAddr());
    return buf;
}

string InetAddress::toIp() const
{
    char buf[64] = "";
    sockets::toIp(buf, sizeof(buf), getSockAddr());
    return buf;
}

uint16_t InetAddress::toPort() const
{
    //TODO: do not consider ipv6
    return ntohs(M_addr.sin_port);
}

static __thread char t_resolveBuffer[64 * 1024];

bool InetAddress::resolve(StringArg hostname, InetAddress *out)
{
    assert(out != NULL);
    struct hostent hent;
    struct hostent *he = NULL;
    int herror = 0;
    bzero(&hent, sizeof(hent));

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof(t_resolveBuffer), &he, &herrno);
    if(ret == 0 && he!= NULL)
    {
        assert(he->h_addrtype == AF_INET && he->length == sizeof(uint32_t));
        out->M_addr.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
        return true;
    }
    else
    {
        if(ret)
        {
            //TODO:
        }
        return false;
    }

}




