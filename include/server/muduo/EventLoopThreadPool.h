#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; } // 设置底层线程的数量

    void start(const ThreadInitCallback &cb = ThreadInitCallback()); // 开启整个事件循环线程

    // 如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops(); // 返回池里的所有loop

    bool started() const { return started_; }

    const std::string name() const { return name_; }

private:
    int numThreads_;
    bool started_;
    std::string name_;

    EventLoop *baseLoop_; // 用户使用muduo创建的loop 如果线程数为1 那直接使用用户创建的loop 否则创建多EventLoop

    int next_;                                              // 轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 所有事件的线程
    std::vector<EventLoop *> loops_;                        // 事件线程的eventloop指针
};
