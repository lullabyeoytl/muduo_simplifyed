#pragma once
#include "Buffer.hpp"
#include "CallBack.hpp"
#include "InetAddress.hpp"
#include "noncopyable.hpp"
#include <atomic>
#include <memory>
#include <string>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
                      noncopyable {
public:
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);

  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }

  const std::string &name() const { return name_; }

  const InetAddress &localAddress() const { return localAddr_; }

  const InetAddress &peerAddress() const { return peerAddr_; }

  bool connected() const { return state_ == kConnected; }

  void send(const void *message, int len);

  void shutdown();

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                size_t highWaterMark) {
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
  }

    // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once

private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void setState(StateE state) {state_ = state;}
  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

  void send(const std::string& buf);
  void sendInLoop(const void *message, size_t len);
  
  void shutdownInLoop();

  EventLoop *loop_;
  const std::string name_;
  const InetAddress localAddr_;
  const InetAddress peerAddr_;
  std::atomic_int state_;
  bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;

  size_t highWaterMark_;

  // read
  Buffer inputBuffer_;
  // write
  Buffer outputBuffer_;
};