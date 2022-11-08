#include "CurrentThread.h"

namespace CurrentThread
{
    //看到是全局变量， 所有线程共享的，加了__thread，或者是C++11的thread_local来定义的话，就是：虽然是全局变量，但是会在每一个线程存储一份拷贝，这个线程对这个变量的更改，别的线程是看不到的。

    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if(t_cachedTid == 0)
        {
            //通过linux系统调用，获取当前线程的tid值
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}