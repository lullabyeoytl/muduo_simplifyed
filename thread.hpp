#pragma once
#include <functional>
#include <thread>
#include <memory>
#include <atomic>
#include <string>
#include "noncopyable.hpp"

class Thread: noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }

    static int numCreated() { return numCreated_; }
private:
    void setDefaultName();
    
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_;


public:

};