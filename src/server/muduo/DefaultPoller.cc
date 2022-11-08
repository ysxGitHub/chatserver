#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

Poller *Poller::newDeaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        return nullptr; // 生产poll的实例
    }
    else
    {
        return new EPollPoller(loop); //生成epoll的实例
    }
}

