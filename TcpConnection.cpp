#include "TcpConnection.hpp"
#include <memory>
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Logger.hpp"
#include "Socket.hpp"
static EventLoop *CheckLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
  }
  return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)), name_(name), state_(kConnecting),
      reading_(true), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), localAddr_(localAddr),
      peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) {
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

  // protect socket
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    printf("TcpConnection destroyed\n");
}

void TcpConnection::handleRead(Timestamp receriveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_ -> fd(), &savedErrno);
    if (n > 0)
    {
        // use member function from enable_shared_from_this
        messageCallback_(shared_from_this(), &inputBuffer_, receriveTime);
    }else if (n == 0)
    {
        // client close, no data
        handleClose();
    }else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        size_t n = outputBuffer_.writeFd(channel_ -> fd(),&savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_ -> disableWriting();
                if(writeCompleteCallback_)
                {
                    //wakeup thread concerning thread, callback
                    loop_ -> queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else {
            LOG_ERROR("TcpConnection::handle write");
        }
    }
    else{
        LOG_ERROR("Connection fd =%d is down, no more writing");
    }
}

void TcpConnection::handleClose()
{
    setState(kDisconnected);
    channel_ ->disableAll();

    TcpConnectionPtr connptr(shared_from_this());
    connectionCallback_(connptr);
    closeCallback_(connptr);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_ -> isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()
            ));
        }
    }
}

void TcpConnection::sendInLoop(const void* message, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected){
        // ! disconnected, give up writing
        return; 
    }
    // channel_ 第一次开始写数据， 而且缓冲区没有待发送的数据
    // 直接发送
    if (!channel_ -> isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), message, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_ -> queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else{
            // error
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno = EPIPE || errno == ECONNRESET)  // sigpipe reset
                {
                    faultError = true;
                }
            }
        }
    }

    // 还有没发完的数据，存到buffer中
    if (!faultError && remaining > 0)
    {   
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ &&  oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            // over highwatermark:  do its callback
            loop_ -> queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        // append to outputBuffer
        outputBuffer_.append(static_cast<const char*>(message) + nwrote, remaining);
        if (!channel_ -> isWriting())
        {
            channel_ -> enableWriting(); 
            // ctl channel event
        }
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    // ctl epollin 事件 to poller
    channel_->enableReading();

    TcpConnectionPtr connptr(shared_from_this());
    connectionCallback_(connptr);
}

void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_ -> disableAll();
        connectionCallback_(shared_from_this());
    }
    // call poller remove this channel
    channel_->remove();
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_ -> runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
  if (!channel_->isWriting())
  {
    // we are not writing
    socket_->shutdownWrite();
  }
}

void TcpConnection::handleError(){
    LOG_ERROR("tcpconnection error");
}