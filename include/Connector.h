#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "InetAddress.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

class Channel;
class EventLoop;

class Connector : boost::noncopyable,
                  public boost::enable_shared_from_this<Connector>
{
public:
    typedef boost::function<void (int sockfd)> NewConnectionCallback;

    Connector(EventLoop* loop, const InetAddress& serverAddr);

    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        M_newConnectionCallback = cb;
    }

    void start(); //can be called in any thread
    void restart();  // must be called in loop thread
    void stop(); // can be called in any thread

    const InetAddress& serverAddress() const 
    {
        return M_serverAddr;
    }

private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30*1000;
    static const int kInitRetryDelayMs = 500;

    void setState(States s)
    {
        M_state = s;
    }
    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop* M_loop;
    InetAddrss M_serverAddr;
    bool M_connect;
    States M_state;
    boost::scoped_ptr<Channel> M_channel;
    NewConnectionCallback M_newConnectionCallback;
    int M_retryDelayMs;
};

#endif 