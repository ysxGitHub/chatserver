#include "Timestamp.h"

#include <time.h>
#include <stdio.h>

Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}

Timestamp Timestamp::now()
{
    return Timestamp(::time(NULL)); // 获取当前时间
}

std::string Timestamp::toString() const
{
    char buf[128] = {0};
    /*
    C 库函数 struct tm *localtime(const time_t *timer) 使用 timer 的值来填充 tm 结构。timer 的值被分解为 tm 结构，并用本地时区表示。
        struct tm {
            int tm_sec;         // 秒，范围从 0 到 59
            int tm_min;         // 分，范围从 0 到 59
            int tm_hour;        // 小时，范围从 0 到 23
            int tm_mday;        // 一月中的第几天，范围从 1 到 31
            int tm_mon;         // 月份，范围从 0 到 11
            int tm_year;        // 自 1900 起的年数
            int tm_wday;        // 一周中的第几天，范围从 0 到 6
            int tm_yday;        // 一年中的第几天，范围从 0 到 365
            int tm_isdst;       // 夏令时
        };
    */
    struct tm *tm_time = ::localtime(&microSecondsSinceEpoch_);
    ::snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
               tm_time->tm_year + 1900,
               tm_time->tm_mon + 1,
               tm_time->tm_mday,
               tm_time->tm_hour,
               tm_time->tm_min,
               tm_time->tm_sec);
    return buf;
}

#if 0
int main()
{
    std::cout << Timestamp::now().toString() << std::endl;
    return 0;
}
#endif
