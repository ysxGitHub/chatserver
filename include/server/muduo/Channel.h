#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

/*
    理清楚 Eventloop、Channel、Poller之间的关系，Reactor模型上对应多路事件分发器
    Channel理解为通道 封装了socket fd和其感兴趣的 event 如 EPOLLIN，EPOLLIN事件 还绑定了poller返回的具体事件

    文件描述符/事件的回调 和 channel 绑定在一起，epoll 需要监听事件是否发生，然后内核将活跃的事件返回用户空间，epoll 被封装在 poller(epollpoller) 中，poller 得到的文件描述符会返回给 eventloop，eventloop会调用 channel 中的事件的回调。eventloop 是 epoll/poll 与 channel 的桥梁
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>; // 事件回调
    using ReadEventCallback = std::function<void(Timestamp)>; // 只读事件的回调

    Channel(EventLoop *loop, int fd); // 构造函数
    ~Channel(); // 析构函数

    // fd 得到poller通知后，处理事件
    // 调用相应的回调方法来处理事件
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当 channel 被手动 remove 掉，channel还在执行回调操作，就是上面这些回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; } // fd所感兴趣的事件
    void set_revents(int revt) { revents_ = revt; } // poller监听事件，设置channel的fd相应事件

    // 设置fd相应的事件状态，要让fd对这个事件感兴趣
    // update就是调用epoll-ctrl, 通知poller把fd感兴趣的事件添加到fd上
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() {events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    //返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const {return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop *ownerLoop() { return loop_; } // 当前channel属于哪个eventLoop
    void remove(); // 删除channel

private:
    void update(); //更新，内部对象调用
    void handleEventWithGuard(Timestamp receiveTime); // 受保护的处理事件

private:
    // 表示当前fd 和其状态，是没有对任何事件感兴趣，还是对读或者写感兴趣
    static const int kNoneEvent;  // 都不感兴趣
    static const int kReadEvent;  // 读事件
    static const int kWriteEvent; // 写事件

    EventLoop *loop_; // 事件循环

    const int fd_; // fd, Poller监听的对象
    int events_;   // 注册fd感兴趣的事件
    int revents_;  // poller返回的具体发生的
    int index_;    // 文件描述符状态

    std::weak_ptr<void> tie_;
    bool tied_;

    //因为channel通道里面能够获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作
    //这些回调是用户设定的，通过接口传给channel来负责调用 ，channel才知道fd上是什么事件
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
