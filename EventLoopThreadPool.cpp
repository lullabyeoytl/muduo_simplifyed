#include "EventLoopThreadPool.hpp"
#include "EventLoopThread.hpp"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop), name_(nameArg), started_(false), numThreads_(0), next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    started_ = true;
    loops_.clear();

    for(int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }

    // only have one thread, no need to create new thread, use baseLoop_ directly
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    std::vector<EventLoop*> allLoops;
    allLoops.reserve(loops_.size() + 1);
    allLoops.push_back(baseLoop_);
    allLoops.insert(allLoops.end(), loops_.begin(), loops_.end());
    return allLoops;
}

EventLoop *EventLoopThreadPool::getNextLoop() {
    EventLoop *loop = baseLoop_;

    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }

    return loop;
}
