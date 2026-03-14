#include "acceptor.hpp"
#include "InetAddress.hpp"
#include "Logger.hpp"
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static int createNonblocking() {
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_FATAL("%s:%s:%d: listen socket create err: %d\n", __FILE__,
              __FUNCTION__, __LINE__, errno);
  }
  return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reuseport)
    : loop_(loop), acceptSocket_(createNonblocking()),
      acceptChannel_(loop, acceptSocket_.fd()), listening_(false) {
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bindAddress(listenAddr);
  // listenfd 可读并不表示“恰好有 1 个新连接”，而是表示 accept 队列里
  // 至少有 1 个已经完成三次握手的连接可取；真正有多少个，要靠 accept
  // 自己不断取到队列为空为止。
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

void Acceptor::handleRead() {
  // 这里用循环，是为了把当前 listenfd 的 accept 队列一次性取空。
  //
  // 原因：
  // 1. 一次 EPOLLIN 可能对应 backlog 里“多个”新连接，而不是一个。
  // 2. 如果这里只 accept 一次，剩余连接会继续留在队列里；在当前 LT 模式下，
  //    后续还会再次收到可读通知，所以功能上通常没错，但会多一次/多次 epoll
  //    唤醒与调度，效率更差。
  // 3. 如果未来切到 ET 模式，那就必须循环 accept 到 EAGAIN，否则可能把
  //    剩余连接永远留在队列里，后面也收不到新的边沿通知。
  //
  // 所以这里采用更稳妥的写法：non-blocking accept + 循环 drain。
  while (true) {
    InetAddress peerAddress;
    int connfd = acceptSocket_.accept(&peerAddress);
    if (connfd >= 0) {
      if (newConnectionCallback_) {
        newConnectionCallback_(connfd, peerAddress);
      } else {
        // no callback, unable to serve the new connection
        ::close(connfd);
      }
      continue;
    }

    // 非阻塞 listenfd ，EAGAIN/EWOULDBLOCK 不是异常，
    // 当前 accept 队列已经被取空了，本轮处理到此结束。
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }

    LOG_ERROR("%s:%s:%d accept err: %d \n", __FILE__, __FUNCTION__, __LINE__,
              errno);
    if (errno == EMFILE) {
      LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__,
                __LINE__);
    }
    break;
  }
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void Acceptor::listen() {
  listening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}
