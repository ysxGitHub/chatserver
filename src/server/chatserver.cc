#include "chatservice.hpp"
#include "chatserver.hpp"
#include "nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <functional>

// 初始化聊天服务器对象，构造 TCPServer
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const std::string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, std::placeholders::_1));

    // 注册消息回调
    _server.setMessageCallback(
        std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置线程数量4
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开链接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseExpcepton(conn); // 客户端异常关闭
        conn->shutdown();                                    // 关闭文件描述符
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    std::string buf = buffer->retrieveAllAsString(); // 转成字符串接收

    // 测试，添加 json 打印代码
    std::cout << buf << std::endl;
    std::cout << std::endl;

    // 数据的反序列化
    nlohmann::json js = nlohmann::json::parse(buf);
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过 js["msgid"] 获取=》业务handler处理器（在业务模块事先绑定好的）=》conn  js  time传给你
    // 转成整型
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); // json

    // 回调消息绑定好的事件处理器，来执行相应的业务处理，一个ID一个操作
    msgHandler(conn, js, time);
}
