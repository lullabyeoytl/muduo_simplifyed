#include "thread.hpp"
#include <semaphore.h>
#include "CurrentThread.hpp"

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), thread_(nullptr), tid_(0), func_(std::move(func)), name_(name) {
    setDefaultName();
}

void Thread::setDefaultName() {
    if (name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", numCreated_ + 1);
        name_ = buf;
    }
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();           // 线程分离，允许线程在后台运行，直到它完成
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sem);             // 释放信号量，通知主线程tid已获取
        func_();
    }));

    // wait getting new spawn thread tid
    sem_wait(&sem);
}

void Thread::join() {
    joined_ = true;
    thread_->join();
}