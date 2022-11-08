#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <mutex>

#include "TcpConnection.h"
#include "Timestamp.h"
#include "nlohmann/json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"


//表示处理消息的事件回调方法类型，事件处理器，派发3个东西
using MsgHandler = std::function<void(const TcpConnectionPtr &, nlohmann::json &, Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
    // 禁止 copy 构造
    ChatService(const ChatService &cs) = delete;
    // 禁止 = 赋值构造
    ChatService &operator=(const ChatService &cs) = delete;

    // 获取单例对象的接口函数
    static ChatService *instance();

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);
    // 处理注册业务
    void regist(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseExpcepton(const TcpConnectionPtr &conn);
    // 服务器异常，业务重置方法
    void reset();

    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);

    // 创建组群业务
    void createGroup(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);
    // 加入组群业务
    void addGroup(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);
    // 组群聊天业务
    void groupChat(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time);

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, std::string);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);


private:
    // 单例
    ChatService();

private:
    //存储消息id和其对应的业务处理方法，消息处理器的一个表，写消息id对应的处理操作
    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接，用户的id， TcpConnectionPtr
    std::unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap的线程安全
    std::mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel; //OfflineMsgModel
    FriendModel _friendModel;
    GroupModel _groupModel;

    // redis操作对象
    Redis _redis;
};

#endif