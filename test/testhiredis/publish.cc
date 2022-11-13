#include <iostream>
#include <string>

#include <hiredis/hiredis.h>
using namespace std;

int main(int argc, char const *argv[])
{
    // 负责publish发布消息的上下文连接
    redisContext *publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == publish_context) {
        cerr << "connect redis failed!" << endl;
        return -1;
    }

    int channel = 1;
    string message = "hahahaha!";

    redisReply *reply = (redisReply *)redisCommand(publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return -1;
    }
    freeReplyObject(reply);


    if (publish_context != nullptr) {
        redisFree(publish_context);
    }

    return 0;
}

