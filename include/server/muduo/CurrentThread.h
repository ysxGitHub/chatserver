#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid; // 保存tid缓存，因为系统调用非常耗时，拿到tid后将其保存

    void cacheTid();

    inline int tid() //内联函数，在当前文件起作用
    {
        if(__builtin_expect(t_cachedTid == 0, 0)) // 还没有获取过当前线程的id
        {
            cacheTid();
        }
        return t_cachedTid;
    }

}