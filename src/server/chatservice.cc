#include <vector>

#include "chatservice.hpp"
#include "public.hpp"
#include "Logger.h"

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 构造方法，注册消息以及对应的 Handler 回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap[LOGIN_MSG] = std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[LOGINOUT_MSG] = std::bind(&ChatService::loginout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[REGIST_MSG] = std::bind(&ChatService::regist, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[ONE_CHAT_MSG] = std::bind(&ChatService::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[ADD_FRIEND_MSG] = std::bind(&ChatService::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[DELETE_FRIEND_MSG] = std::bind(&ChatService::deleteFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap[CREATE_GROUP_MSG] = std::bind(&ChatService::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[ADD_GROUP_MSG] = std::bind(&ChatService::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[EXIT_GROUP_MSG] = std::bind(&ChatService::exitGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap[GROUP_CHAT_MSG] = std::bind(&ChatService::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    // 连接 redis 服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid 没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) //找不到
    {
        // 返回一个默认的处理器，空操作，=按值获取
        return [=](const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
        {
            LOG_ERROR("msgid: %d can not find handler!\n", msgid);
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务  id  pwd  检测 pwd
void ChatService::login(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    // LOG_INFO("do login service!!!\n");
    int id = js["id"].get<int>();     // 获取 id 号
    std::string pwd = js["password"]; // 获取密码

    User user = _userModel.query(id); // 查找 id 对应用户的信息 并将 id name password state 保存到 user 临时栈空间中

    if (user.getId() == id && user.getPwd() == pwd) // 查出来了，登录成功
    {
        if (user.getState() == "online") // 该用户已经登录，不允许重复登录
        {
            nlohmann::json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else // 登录成功，更新用户状态信息 state offline=>online
        {
            // 登录成功，记录用户连接信息，要考虑线程安全，因为多线程访问
            {
                std::lock_guard<std::mutex> lock(_connMutex);
                _userConnMap[id] = conn;
            }

            // id 用户登录成功后，向 redis 订阅 channel(id)
            _redis.subscribe(id);

            user.setState("online");
            _userModel.updateState(user);

            nlohmann::json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            std::vector<std::string> vec = _offlineMsgModel.query(id);
            if (!vec.empty()) // 有离线消息
            {
                response["offlinemsg"] = vec; // json 库可以和容器之间序列化和反序列化
                _offlineMsgModel.remove(id);  // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
            }

            // 查询该用户的好友信息并返回
            std::vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                std::vector<std::string> vec2;
                for (User &user : userVec)
                {
                    nlohmann::json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            std::vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                std::vector<std::string> groupV;
                for (auto &group : groupuserVec)
                {
                    nlohmann::json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    std::vector<std::string> userV;
                    for (auto &user : group.getUsers())
                    {
                        nlohmann::json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    }
    else if (user.getId() != -1) // 该用户不存在，用户存在但是密码错误，登录失败
    {
        nlohmann::json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
    else
    {
        // 该用户不存在 或用户存在但密码错误 登录失败   发送消息包含字段 msgid errno errmsg
        nlohmann::json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1; // errno = -1表示没查出来
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 处理注册业务  name  password
void ChatService::regist(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    // LOG_INFO("do regist service!!!\n");

    std::string name = js["name"];    //获取名字
    std::string pwd = js["password"]; //获取密码

    User user; // 创建用户对象
    user.setName(name);
    user.setPwd(pwd);

    bool state = _userModel.insert(user); // 新用户的插入

    if (state) // 插入成功
    {
        // 注册成功
        nlohmann::json response;
        response["msgid"] = REGIST_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump()); // 回调 ，返回 json 字符串
    }
    else
    {
        nlohmann::json response;
        response["msgid"] = REGIST_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::clientCloseExpcepton(const TcpConnectionPtr &conn)
{
    User user;

    {
        std::lock_guard<std::mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在 redis 中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int toid = js["toid"].get<int>(); // 获取对方的 id 号

    {
        std::lock_guard<std::mutex> lock_(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid 在线，转发消息  服务器主动推送消息给 toid 用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询 toid 是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid 不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把 online 状态的用户，设置成 offline
    _userModel.resetState();
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int userid = js["id"].get<int>();         // 当前用户的 id
    int friendid = js["friendid"].get<int>(); // 添加好友的 id

    //存储好友信息
    _friendModel.insert(userid, friendid);
}

// 删除好友业务
void ChatService::deleteFriend(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int userid = js["id"].get<int>();         // 当前用户的 id
    int friendid = js["friendid"].get<int>(); // 删除好友的 id
    // 删除好友信息
    _friendModel.del(userid, friendid);
}

// 创建组群业务
void ChatService::createGroup(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 创建群的用户的 id
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入组群业务
void ChatService::addGroup(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

void ChatService::exitGroup(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.exitGroup(userid, groupid);
}

// 组群聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    // 查询这个用户所在群组的其他用户 id
    std::vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    std::lock_guard<std::mutex> lock(_connMutex); // 不允许其他人在 map 里面增删改查

    for (auto id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) // 证明在线，直接转发
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询 toid 是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, nlohmann::json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在 redis 中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 从 redis 消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, std::string msg)
{
    // 这里考虑到一种情况：当接收到
    std::lock_guard<std::mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}
