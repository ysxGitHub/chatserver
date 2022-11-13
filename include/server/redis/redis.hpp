#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接 redis 服务器
    bool connect();

    // 向 redis 指定的通道 channel 发布消息
    bool publish(int channel, std::string message);

    // 向 redis 指定的通道 subscribe 订阅消息
    bool subscribe(int channel);

    // 向 redis 指定的通道 unsubscribe 取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();

    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(std::function<void(int, std::string)> fn);

private:
    // hiredis 同步上下文对象，负责 publish 消息，相当于客户端一个 redis-client
    redisContext *_publish_context;

    // hiredis 同步上下文对象，负责 subscribe 消息，相当于客户端一个 redis-client ，
	// _publish 和 _subcribe 不能在一个上下文处理 ,会阻塞
    redisContext *_subcribe_context;

    // 回调操作，收到订阅的消息，给 service 层上报
    std::function<void(int, std::string)> _notify_message_handler;
};

#endif