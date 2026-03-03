#include "../InetAddress.hpp"
#include "../Logger.hpp"
#include "../EventLoop.hpp"
#include "../CallBack.hpp"
#include "../TcpServer.hpp"
#include "../TcpConnection.hpp"
#include "../timestamp.hpp"
#include <unistd.h>

// using namespace muduo;
// using namespace muduo::net;

class EchoServer
{
 public:
  EchoServer(EventLoop* loop,
             const InetAddress& listenAddr);

  void start();  // calls server_.start();

 private:
  void onConnection(const TcpConnectionPtr& conn);

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp time);

  TcpServer server_;
};

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listenAddr)
  : server_(loop, listenAddr, "EchoServer")
{
  server_.setConnectionCallback(
      std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
  server_.setMessageCallback(
      std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void EchoServer::start()
{
  server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        LOG_INFO("new connection");
    }
    else
    {
        LOG_INFO("connection down");
    }
}
void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
  std::string msg(buf->retrieveAllAsString());
  LOG_INFO ("%s echo %d bytes, data received at %s", conn->name().c_str(), msg.size(), time.toString().c_str());
  conn->send(msg);
}



int main()
{
  LOG_INFO ("pid = %d", getpid());
  EventLoop loop;
  InetAddress listenAddr(2007);
  EchoServer server(&loop, listenAddr);
  server.start();
  loop.loop();
}
