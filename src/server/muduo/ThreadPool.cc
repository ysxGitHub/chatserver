#include <string>
#include "Logger.h"

#include "ThreadPool.h"

ThreadPool::ThreadPool(const std::string &nameArg)
    : name_(nameArg)
    , maxQueueSize_(0)
    , running_(false)
{
}

ThreadPool::~ThreadPool()
{
    if(running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    if(!threads_.empty())
    {
        LOG_ERROR("ThreadPool is not empty!\n");
    }

    running_ = true;

    threads_.reserve(numThreads);
    for(int i = 0; i < numThreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof(id), "%d", i+1);
        threads_.emplace_back(new Thread(
            std::bind(&ThreadPool::runInThread, this), name_ + id));
        threads_[i]->start();
    }

    if(numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}

void ThreadPool::stop()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
        notEmpty_.notify_all();
        notFull_.notify_all();
    }
    for(auto &thr : threads_)
    {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
}

void ThreadPool::run(Task task)
{
    if(threads_.empty())
    {
        task();
    }
    else
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (isFull() && running_)
        {
            notFull_.wait(lock);
        }

        if(!running_) return ;

        if(isFull())
        {
            LOG_ERROR("ThreadPool is Full!\n");
        }

        queue_.push_back(std::move(task));
        notEmpty_.notify_one();
    }
}

ThreadPool::Task ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while(queue_.empty() && running_)
    {
        notEmpty_.wait(lock);
    }

    Task task;
    if(!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        if(maxQueueSize_ > 0)
        {
            notFull_.notify_one();
        }
    }
    return task;
}

bool ThreadPool::isFull() const
{
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
    if(threadInitCallback_)
    {
        threadInitCallback_();
    }

    while(running_)
    {
        Task task(take());
        if(task)
        {
            task();
        }
    }
}



