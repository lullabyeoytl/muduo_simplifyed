#include "../CallBack.hpp"
#include "../EventLoop.hpp"
#include "../InetAddress.hpp"
#include "../Logger.hpp"
#include "../TcpConnection.hpp"
#include "../TcpServer.hpp"

#include <cstdlib>
#include <ctime>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unistd.h>

class HttpBenchServer {
public:
  HttpBenchServer(EventLoop *loop, const InetAddress &listenAddr,
                  TcpServer::Option option, int threadNum)
      : server_(loop, listenAddr, "HttpBenchServer", option) {
    server_.setThreadNum(threadNum);
    server_.setConnectionCallback(
        std::bind(&HttpBenchServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&HttpBenchServer::onMessage, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3));
  }

  void start() { server_.start(); }

private:
  std::string buildResponse() const {
    char date[128] = {0};
    std::time_t now = std::time(nullptr);
    std::tm gmTime;
    gmtime_r(&now, &gmTime);
    std::strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", &gmTime);

    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Server: nginx\r\n"
        << "Date: " << date << "\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: 12\r\n"
        << "Connection: keep-alive\r\n"
        << "\r\n"
        << "hello world\n";
    return oss.str();
  }

  void onConnection(const TcpConnectionPtr &conn) {
    std::lock_guard<std::mutex> lock(requestMutex_);
    if (!conn->connected()) {
      requestBuffers_.erase(conn->name());
    }
  }

  void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp) {
    // notice that the callback of one tcp connection only triggered in current thread
    // no need to use request_mutex_ in onMessage function
    std::string pendingInput = buf->retrieveAllAsString();
    std::string pending;
    
    pending = requestBuffers_[conn->name()];
    pending.append(pendingInput);

    while (true) {
      const std::string::size_type headerEnd = pending.find("\r\n\r\n");
      if (headerEnd == std::string::npos) {
        break;
      }

      conn->send(buildResponse());
      pending.erase(0, headerEnd + 4);
    }

    requestBuffers_[conn->name()] = pending;

  }

  TcpServer server_;
  std::mutex requestMutex_;
  std::unordered_map<std::string, std::string> requestBuffers_;
};

namespace {

TcpServer::Option parseOption(const char *arg) {
  if (arg != nullptr && std::string(arg) == "noreuseport") {
    return TcpServer::kNoReusePort;
  }
  return TcpServer::kReusePort;
}

} // namespace

int main(int argc, char *argv[]) {
  const TcpServer::Option option = parseOption(argc > 1 ? argv[1] : nullptr);
  const uint16_t port =
      static_cast<uint16_t>(argc > 2 ? std::atoi(argv[2]) : 2008);
  const int threadNum = argc > 3 ? std::atoi(argv[3]) : 12;

  LOG_INFO("pid = %d mode=%s port=%u threads=%d", getpid(),
           option == TcpServer::kReusePort ? "reuseport" : "noreuseport", port,
           threadNum);

  EventLoop loop;
  InetAddress listenAddr(port);
  HttpBenchServer server(&loop, listenAddr, option, threadNum);
  server.start();
  loop.loop();
}
