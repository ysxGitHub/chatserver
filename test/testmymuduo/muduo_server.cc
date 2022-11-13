#include "mymuduo/EventLoop.h"
#include "mymuduo/TcpServer.h"
#include "mymuduo/ThreadPool.h"

#include <functional>
#include <string>
#include <iostream>

class ChatServer//TCPServer
{
public:
    ChatServer(EventLoop *loop,               //事件循环
               const InetAddress &listenAddr, //muduo封装好的，绑定IP+Port
               const std::string &nameArg)//给TCPserver一个名字
        : _server(loop, listenAddr, nameArg), _loop(loop)//没有默认构造哦
    {
        //给服务器注册用户连接的创建和断开的回调，回调就是对端的相应事件发生了告诉网络库 ，然后网络库告诉我 ，我在回调函数开发业务
        _server.setConnectionCallback(
            std::bind(&ChatServer::onConnection, this, std::placeholders::_1));//绑定this对象到这个方法中，_1是参数占位符

        //给服务器注册用户读写事件的回调
        _server.setMessageCallback(
            std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));//绑定this对象到这个方法中

        //设置服务器端的线程数量 1个I/O线程（监听新用户的连接事件）， 3个worker线程
        //不设置的话，就1个线程而已，要处理连接又要处理业务
        _server.setThreadNum(4);//设置4个线程，1个I/O线程，3个worker线程
    }

    void start()//开启事件循环
    {
        _server.start();
    }

private:
    //专门处理：用户的连接创建和断开 epoll listenfd accept
    //如果有新用户的连接或者断开，muduo库就会调用这个函数
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())//连接 ， peerAddress()对端的地址 localAddress() 本地的地址
        {
            std::cout << conn->peerAddress().toIpPort()
                      << " -> "
                      << conn->localAddress().toIpPort()
                      << " state:online" << std::endl;
        }
        else//断开
        {
            std::cout << conn->peerAddress().toIpPort()
                      << " -> "
                      << conn->localAddress().toIpPort()
                      << " state:offline" << std::endl;
            conn->shutdown();//相当于这些close(fd)
            //_loop->quit();
        }
    }

    //专门处理：用户的读写事件，muduo库去调用这个函数
    void onMessage(const TcpConnectionPtr &conn, //连接，通过这个连接可以读写数据
                   Buffer *buffer,               //缓冲区，提高数据收发的性能
                   Timestamp time)               //接收到数据的时间信息
    {
        std::string buf = buffer->retrieveAllAsString();//收到的数据放到这个字符串中
        std::cout << "recv data:"
                  << buf
                  << "time: "
                  << time.toString()
                  << std::endl;
        std::cout<<std::endl;
        conn->send(buf);//返回 ，收到什么发送什么
    }

    TcpServer _server;//第一步
    EventLoop *_loop; //第二步相当于 epoll 事件循环的指针，有事件发生，loop上报
};

int main()
{
    EventLoop loop;//相当于像是创建了epoll
    InetAddress addr(6001, "192.168.39.131");//IP地址，端口号
    ChatServer server(&loop, addr, "ChatServer");

    server.start();//listenfd通过 epoll_ctl 添加到 epoll
    loop.loop();//相当于epoll_wait，以阻塞方式等待新用户连接，已连接用户的读写事件等
}





