#include "Socket.hpp"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "InetAddress.hpp"
#include "Logger.hpp"

void Socket::bindAddress(const InetAddress& localaddr)
{
    if (::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(struct sockaddr_in)) != 0)
    {
        // handle error
        LOG_FATAL("bind sockfd:%d fail", sockfd_);
    }
}

void Socket::listen()
{
    if (::listen(sockfd_, SOMAXCONN) < 0)
    {
        // handle error
        LOG_FATAL("listen sockfd:%d fail", sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr)
{
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0 && on)
    {
        LOG_FATAL("SO_REUSEPORT set failed on sockfd:%d", sockfd_);
    }
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}
