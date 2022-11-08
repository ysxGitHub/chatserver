#pragma once

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

#include "noncopyable.h"

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() { return started_; }
    pid_t tid() const {return tid_; }

    const std::string &name() const { return name_; }
    static int numCreated() { return numCreated_; }


private:
    bool joined_;

    bool started_;
    pid_t tid_; // 在线程创建时再绑定
    std::string name_;
    static std::atomic_int numCreated_;

    ThreadFunc func_; // 线程回调函数

    std::shared_ptr<std::thread> thread_;

    void setDefaultName();
};



