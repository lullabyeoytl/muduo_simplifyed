#include <functional>
#include "Channel.hpp"
#include "Socket.hpp"
class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public: 
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }
    bool listenning() const { return listening_; }
    void listen();

private: 
    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    void handleRead();

};