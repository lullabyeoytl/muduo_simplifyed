#pragma once

#include <functional>
#include <memory>
#include "noncopyable.hpp"
#include "timestamp.hpp"

class EventLoop;


class Channel: noncopyable
{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;
    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    void setReadCallback(const ReadEventCallback& cb) { readCallback_ = std::move(cb); }

    void setWriteCallback(const EventCallback& cb) { writeCallback_ = std::move(cb); }

    void setCloseCallback(const EventCallback& cb) { closeCallback_ = std::move(cb); }

    void setErrorCallback(const EventCallback& cb) { errorCallback_ = std::move(cb); }

    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }

    int events() const { return events_; }

    void set_revents(int revt) { revents_ = revt; }

    bool isNoneEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }

    void remove();

private:
    // current fd status
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;          // fd listened by poller
    int events_;          // fd currently interested in
    int revents_;       // fd returned by epoll_wait
    int index_;        // used by Poller

    std::weak_ptr<void> tie_;     // 指向持有Channel的对象，回调前通过tie_.lock()提升为shared_ptr
    bool tied_;

    // 最终发生的具体事件revents,调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    void update();

    void handleEventWithGuard(Timestamp receiveTime);
};