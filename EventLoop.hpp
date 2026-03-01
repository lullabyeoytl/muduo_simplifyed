#pragma once 
#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include "noncopyable.hpp"
#include "timestamp.hpp"
#include "CurrentThread.hpp"


class Channel;
class Poller;

class EventLoop: noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    void wakeup();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);
    
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
private:
    void handleRead();   // waked up
    void doPendingFunctors();   // execute pending functors
    
    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;
    std::atomic_bool quit_;  
    std::atomic_bool callingPendingFunctors_;
    const pid_t threadId_;
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    std::atomic_bool caliingPendingFunctors_;    // wheter the EventLoop is calling pending functors
    std::vector<Functor> pendingFunctors_;      // save all callback functors which are need to be called in loop thread
    std::mutex mutex_;
};