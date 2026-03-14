#pragma once
#include <map>
#include <memory>
#include <functional>
#include <string>
#include <atomic>
#include <mutex>
#include <vector>
#include "EventLoop.hpp"
#include "InetAddress.hpp"
#include "acceptor.hpp"
#include "EventLoopThreadPool.hpp"
#include "noncopyable.hpp"
#include "CallBack.hpp"

class TcpServer : noncopyable 
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string &nameArg, Option option = kNoReusePort);

    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback& cb)
    {
        threadInitCallback_ = cb;
    }

    void setThreadNum(int numThreads){
        threadPool_->setThreadNumber(numThreads);
    };

    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb){
        messageCallback_ = cb;
    }

    void start();

private:
    void newConnection(EventLoop* acceptLoop, int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;
    const InetAddress listenAddr_;
    const std::string IpPort_;
    const std::string name_;
    const bool reusePort_;
    std::vector<std::unique_ptr<Acceptor>> acceptors_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    ThreadInitCallback threadInitCallback_;
    std::atomic_int started_;

    std::atomic_int nextConnId_;
    std::mutex connectionMutex_;
    ConnectionMap connections_;
};
