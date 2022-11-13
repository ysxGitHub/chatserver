#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 时间循环类，主要包含了两个大模块 （Channel Poller（epoll的抽象））
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>; // 定义一个回调函数

    EventLoop();
    ~EventLoop();

    // 开启事务循环
    void loop();

    // 退出时间循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前 loop 中执行 callback
    void runInLoop(Functor cb);

    // 把 callback 放入队列中，唤醒 loop 所在的线程，执行 callback
    void queueInLoop(Functor cb);

    // 通过 eventfd 唤醒用来唤醒 loop 所在的(子)线程的
    void wakeup();

    // EventLoop 的方法 =》 Poller 的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断 EventLoop 对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); } // threadId_为EventLoop创建时的线程id CurrentThread::tid()为当前线程id

private:
    void handleRead();        // 给eventfd返回的文件描述符 wakeupFd_ 绑定的事件回调 当 wakeup() 时 即有事件发生时 调用 handleRead() 读 wakeupFd_ 的8字节 同时唤醒阻塞的 epoll_wait
    void doPendingFunctors(); // 执行上层回调

private:
    using ChannelList = std::vector<Channel *>;

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    const pid_t threadId_;     // 记录当前EventLoop是被哪个线程id创建的 即标识了当前EventLoop的所属线

    std::atomic_bool looping_; // 原子操作，通过CAS实现的
    std::atomic_bool quit_;    // 标识退出loop循环

    std::unique_ptr<Poller> poller_; // eventloop所管理的poller
    int wakeupFd_;                   // linux内核的eventfd创建出来的。主要作用，当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel

    std::unique_ptr<Channel> wakeupChannel_; // 包括wakeupFd 和感兴趣的事件

    ChannelList activeChannels_; // eventloop所管理的channel

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctor_;     // 存储loop需要执行的所有的回调操作
    std::mutex mutex_;                        // 互斥锁，用来保护上面vector容器的线程安全操作
};
