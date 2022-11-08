#include <iostream>
#include <string>

#include <hiredis/hiredis.h>
using namespace std;

int main(int argc, char const *argv[])
{
    // 负责subscribe订阅消息的上下文连接
    redisContext *subcribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == subcribe_context) {
        cerr << "connect redis failed!" << endl;
        return -1;
    }

    cout << "connect redis-server success!" << endl;

    int channel = 1;

    if (REDIS_ERR == redisAppendCommand(subcribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return -1;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subcribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return -1;
        }
    }
    cout << "ok, next step" << endl;

    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(subcribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组 即"message"、"订阅的通道号"、"真正的消息"
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            cout << atoi(reply->element[1]->str) << " " << reply->element[2]->str;
        }

        freeReplyObject(reply);
    }

    if (subcribe_context != nullptr) {
        redisFree(subcribe_context);
    }

    return 0;
}

