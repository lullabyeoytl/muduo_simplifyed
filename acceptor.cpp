#include "acceptor.hpp"
#include "InetAddress.hpp"
#include "Logger.hpp"
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
  // 有新用户的连接，要执行一个回调 connfd -> Channel -> subloop
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

void Acceptor::handleRead() {
  InetAddress peerAddress;
  int connfd = acceptSocket_.accept(&peerAddress);
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddress);
    } else {
      // no callback, unable to serve the new connection
      ::close(connfd);
    }
  } else {
    LOG_ERROR("%s:%s:%d accept err: %d \n", __FILE__, __FUNCTION__, __LINE__,
              errno);
    if (errno == EMFILE) {
      LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__,
                __LINE__);
    }
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

