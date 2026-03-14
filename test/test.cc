#include "../InetAddress.hpp"
#include "../Logger.hpp"
#include "../EventLoop.hpp"
#include "../CallBack.hpp"
#include "../TcpServer.hpp"
#include "../TcpConnection.hpp"
#include "../CurrentThread.hpp"
#include "../timestamp.hpp"
#include <cstdlib>
#include <string>
#include <unistd.h>

// using namespace muduo;
// using namespace muduo::net;

class EchoServer
{
 public:
  EchoServer(EventLoop* loop,
             const InetAddress& listenAddr,
             TcpServer::Option option,
             int threadNum,
             bool verbose);

  void start();  // calls server_.start();

 private:
  void onConnection(const TcpConnectionPtr& conn);

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp time);

  TcpServer server_;
  bool verbose_;
};

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       TcpServer::Option option,
                       int threadNum,
                       bool verbose)
  : server_(loop, listenAddr, "EchoServer", option),
    verbose_(verbose)
{
  server_.setThreadNum(threadNum);
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
    if (!verbose_)
    {
        return;
    }

    if (conn->connected())
    {
        LOG_INFO("new connection on tid=%d local=%s peer=%s",
                 CurrentThread::tid(),
                 conn->localAddress().toIpPort().c_str(),
                 conn->peerAddress().toIpPort().c_str());
    }
    else
    {
        LOG_INFO("connection down on tid=%d local=%s peer=%s",
                 CurrentThread::tid(),
                 conn->localAddress().toIpPort().c_str(),
                 conn->peerAddress().toIpPort().c_str());
    }
}
void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
  std::string msg(buf->retrieveAllAsString());
  if (verbose_)
  {
    LOG_INFO ("%s echo %zu bytes, data received at %s", conn->name().c_str(), msg.size(), time.toString().c_str());
  }
  conn->send(msg);
}

namespace {

TcpServer::Option parseOption(const char* arg)
{
  if (arg != nullptr && std::string(arg) == "noreuseport")
  {
    return TcpServer::kNoReusePort;
  }
  return TcpServer::kReusePort;
}

bool parseVerbose(const char* arg)
{
  return arg != nullptr && std::string(arg) == "verbose";
}

}  // namespace

int main(int argc, char* argv[])
{
  const TcpServer::Option option = parseOption(argc > 1 ? argv[1] : nullptr);
  const uint16_t port = static_cast<uint16_t>(argc > 2 ? std::atoi(argv[2]) : 2007);
  const int threadNum = argc > 3 ? std::atoi(argv[3]) : 3;
  const bool verbose = parseVerbose(argc > 4 ? argv[4] : nullptr);

  LOG_INFO ("pid = %d mode=%s port=%u threads=%d verbose=%d",
            getpid(),
            option == TcpServer::kReusePort ? "reuseport" : "noreuseport",
            port,
            threadNum,
            verbose ? 1 : 0);
  EventLoop loop;
  InetAddress listenAddr(port);
  EchoServer server(&loop, listenAddr, option, threadNum, verbose);
  server.start();
  loop.loop();
}
