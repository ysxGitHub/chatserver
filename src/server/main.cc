#include "chatserver.hpp"
#include "chatservice.hpp"
#include "mysqlconnection.hpp"

#include <signal.h>
#include <iostream>

//处理服务器ctrl+c结束后，进行重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        std::cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << std::endl;
    }

    //解析通过命令行参数传递的ip和port
    const char *ip = argv[1];
    uint16_t port = std::atoi(argv[2]);

    signal(SIGINT, resetHandler); // 键盘Ctrl+C

    EventLoop loop;
    InetAddress addr(port, ip);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}
