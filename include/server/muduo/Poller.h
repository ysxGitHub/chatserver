#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;

// muduo 库中多路事件分发器的核心IO复用模块
class Poller
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop 可以通过该接口获取默认的IO复用的具体实现
    static Poller *newDeaultPoller(EventLoop *loop);

protected:
    // map的key:socket fd , value:socket所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
};

// 为什么不把 newDefaultPoller写在Poller.cc？

/*
如果真的把newDefaultPoller写在Poller.cc里面，从语法上来说，没有错误。
但是这个函数是要生成一个具体的I/O复用对象，并返回一个基类的指针。
所以就得用下面这2个头文件，才能去生成一个具体的实例对象并返回回去，这样不合理，因为继承结构中，poller是基类，只能派生类引用基类，基类不能引用派生类

#include "PollPoller.h"
#include "EpollPoller.h"

*/