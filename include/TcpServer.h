#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Atomic.h"
#include "Types.h"
#include "TcpConnection.h"

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details. 

class TcpServer : boost::noncopyable
{
public:

private:


};

#endif
