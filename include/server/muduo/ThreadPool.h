#pragma once

#include <deque>
#include <vector>
#include <mutex>
#include <memory>
#include <string>
#include <functional>
#include <condition_variable>

#include "noncopyable.h"
#include "Thread.h"

class ThreadPool : noncopyable
{
public:
    using Task = std::function<void()>;

    explicit ThreadPool(const std::string &nameArg = std::string("ThreadPool"));
    ~ThreadPool();

    // 必须在start()之前调用。
    void setMaxQueueSize(int maxsize) { maxQueueSize_ = maxsize; }
    void setThreadInitCallback(const Task &cb)
    {
        threadInitCallback_ = cb;
    }

    void start(int numThreads);
    void stop();

    const std::string &name() const { return name_; }

    size_t queueSize() const;

    // 如果maxQueueSize>0，可能会阻塞。
    // 在stop()之后的调用将立即返回。
    // 从C++14开始，C++中没有std::function的纯移动版本。
    // 所以我们不需要重载一个const&和一个&&版本。
    // 像我们在(Bounded)BlockingQueue中做的那样。
    void run(Task f);

private:
    bool isFull() const;
    void runInThread();
    Task take();

private:
    size_t maxQueueSize_;
    Task threadInitCallback_;
    std::string name_;

    mutable std::mutex mutex_;

    std::condition_variable notEmpty_;
    std::condition_variable notFull_;

    std::vector<std::unique_ptr<Thread>> threads_;
    std::deque<Task> queue_;

    bool running_;
};
