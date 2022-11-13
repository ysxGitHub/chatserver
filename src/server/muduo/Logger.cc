#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

// 获取日志唯一的实例对象
Logger& Logger::instance()
{
    static Logger Logger;
    return Logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 写日志  [级别信息] time : msg
void Logger::log(std::string msg)
{
    std::string pre = "";
    switch(logLevel_)
    {
    case INFO:
        pre = "[INFO]";
        break;
    case ERROR:
        pre = "[ERROR]";
        break;
    case FATAL:
        pre = "[FATAL]";
        break;
    case DEBUG:
        pre = "[DEBUG]";
        break;
    default:
        break;
    }
    // 打印时间和msg
    std::cout << pre + Timestamp::now().toString() << " : " << msg << std::endl;
}
