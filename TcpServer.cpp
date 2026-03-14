#include "TcpServer.hpp"
#include "Logger.hpp"
#include <strings.h>
#include "TcpConnection.hpp"

EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("TcpServer requires a non-null EventLoop");
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string &nameArg, Option option)
    : loop_(CheckLoopNotNull(loop)),
      listenAddr_(listenAddr),
      IpPort_(listenAddr.toIpPort()),
      name_(nameArg),
      reusePort_(option == kReusePort),
      threadPool_(new EventLoopThreadPool(loop, name_)),
    //   connectionCallback_(defaultConnectionCallback),
    //   messageCallback_(defaultMessageCallback),
    //   writeCompleteCallback_(defaultWriteCompleteCallback),
      threadInitCallback_(),
      started_(0),
      nextConnId_(1)
{
}

TcpServer::~TcpServer(){}

void TcpServer::newConnection(EventLoop* acceptLoop, int sockfd, const InetAddress& peerAddr)
{
    EventLoop* ioLoop = reusePort_ ? acceptLoop : threadPool_->getNextLoop();

    char buf[32];
    int connId = nextConnId_.fetch_add(1);
    snprintf(buf, sizeof buf, ":%s#%d", IpPort_.c_str(), connId);
    std::string connName = name_ + buf;

    // LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("::getsockname error");
    }
    InetAddress localAddr(local);

    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        connections_[connName] = conn;
    }

    // set by user
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::start()
{
    if (started_++ == 0)
    {
        threadPool_->start(threadInitCallback_);

        std::vector<EventLoop*> acceptLoops;
        if (reusePort_)
        {
            // reusePort -> thread set one acceptor
            acceptLoops = threadPool_->getAllLoops();
        }
        else
        {
            // if not: onlu set base loop for accepting 
            acceptLoops.push_back(loop_);
        }

        acceptors_.reserve(acceptLoops.size());
        for (EventLoop* acceptLoop : acceptLoops)
        {
            std::unique_ptr<Acceptor> acceptor(new Acceptor(acceptLoop, listenAddr_, reusePort_));
            acceptor->setNewConnectionCallback(
                std::bind(&TcpServer::newConnection, this, acceptLoop, std::placeholders::_1, std::placeholders::_2));
            acceptors_.push_back(std::move(acceptor));
        }

        for (size_t i = 0; i < acceptLoops.size(); ++i)
        {
            acceptLoops[i]->runInLoop(std::bind(&Acceptor::listen, acceptors_[i].get()));
        }
    }
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn)
    );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    // LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
        // name_.c_str(), conn->name().c_str());

    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        connections_.erase(conn->name());
    }
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}
