#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew = -1;    // 某个channel还未添加到Poller   // channel的成员index_初始化-1
const int kAdded = 1;   // 某个channel已经添加到Poller
const int kDeleted = 2; // 某个channel已经从Poller删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) // 创建epoll树
    , events_(kInitEventListSize) // vector<epoll_event>(16)

{
    if(epollfd_<0)
    {
        LOG_FATAL("epoll_create error:%d\n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_); // 注销epoll树
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 由于频繁调用poll 实际上应该用LOG_DEBUG输出日志更为合理，当遇到并发场景 关闭DEBUGE日志提升效率
    LOG_DEBUG("func=%s => fd total count: %lu\n", __FUNCTION__, channels_.size());

    //events_.begin()返回首元素的迭代器（数组），也就是首元素的地址，是面向对象的，要解引用，就是首元素的值，然后取地址就是vector底层数组的起始地址
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);

    int saveErrno = errno; //全局的变量errno，poll可能在多个线程eventloop被调用，所以有局部变量存起来

    Timestamp now(Timestamp::now()); //获取当前时间

    if(numEvents > 0) //表示有已经发生相应事件的个数
    {
        LOG_INFO("%d events happned\n", numEvents); // 应该用LOG_DEBUG
        fillActiveChannel(numEvents, activeChannels);
        if(numEvents == events_.size()) //所有的监听的event都发生事件了, 扩容操作
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0) //epoll_wait这一轮监听没有事件发生，timeout超时了
    {
        LOG_DEBUG("%s timeout!\n", __FUNCTION__); // modify
    }
    else
    {
        if(saveErrno != EINTR) //不等于外部的中断 ，是由其他错误类型引起的
        {
            errno = saveErrno; //适配，把errno重置成当前loop之前发生的错误的值
            LOG_ERROR("EPollPoller::poll() err!\n");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/**
 *            EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap  <fd, channel*>   epollfd
 */
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted) // 未添加或者已删除
    {
        if(index == kNew) //未添加，键值对写入map中
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        else // index == kAdd
        {
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel); // 相当于调用epoll_ctl,添加一个channel到epoll中
    }
    else //channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent()) // 已经对任何事件不感兴趣，不需要poller帮忙监听了
        {
            update(EPOLL_CTL_DEL, channel); //删除已注册的channel的感兴趣的事件
            channel->set_index(kDeleted); // 删掉
        }
        else // 包含了fd的事件，感兴趣
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//从poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd); // 从map中删除

    LOG_DEBUG("func=%s => fd=%d\n", __FUNCTION__, fd); // modify

    int index = channel->index();
    if(index==kAdded) //如果已注册过
    {
        update(EPOLL_CTL_DEL, channel); // 通过epoll_ctl删除
    }
    channel->set_index(kNew); // 设置为未添加状态
}

// 填写活跃的链接
void EPollPoller::fillActiveChannel(int numEvents, ChannelList *activeChannels) const
{
    for(int i=0; i<numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr); // 类型转换
        channel->set_revents(events_[i].events);
        activeChannels->emplace_back(channel); //EventLoop 就拿到了它的poller给它返回的所以发生事件的channel列表了， 至于EventLoop拿到这些channel干什么事情，可以看EventLoop的代码
    }
}

// 更新channel通道 其实就是调用epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    bzero(&event, sizeof(event));

    int fd = channel->fd();

    event.events = channel->events(); // 返回的就是fd所感兴趣的事件
    event.data.fd = fd;
    event.data.ptr = channel; // 绑定的参数

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0) // 把fd相关事件更改
    {
        // 出错了
        if(operation == EPOLL_CTL_DEL) // 没有删掉
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else //添加或者更改错误，这个会自动exit
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}



