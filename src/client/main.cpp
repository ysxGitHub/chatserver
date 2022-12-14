#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>

#include "nlohmann/json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息，记录哪个用户在使用这个聊天客户端
User g_currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
std::atomic_bool g_isLoginSuccess{false};

// 接收线程 控制台应用程序，接收用户的手动输入，用户不输入 cin 就阻塞住，所以要2个线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime();
// 登录成功，才能进入主聊天页面程序
void mainMenu(int);
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 聊天客户端程序实现，main 线程用作发送线程，子线程用作接收线程
int main(int argc, char const *argv[])
{
    if (argc < 3) // 判断命令的个数，客户端启动要输入命令行的 ip 地址和端口 port
    {
        std::cerr << "command invalid! example: ./ChatClient 127.0.0.1 6001" << std::endl;
        ::exit(-1);
    }

    //解析通过命令行参数传递的 ip 和 port
    const char *ip = argv[1];
    uint16_t port = std::atoi(argv[2]);

    // 创建 client 端的 socket
    int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        std::cerr << "socket create error" << std::endl;
        ::exit(-1);
    }

    // 填写 client 需要连接的 server 信息 ip+port
    sockaddr_in server;
    ::bzero(&server, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client 和 server 进行连接
    if (-1 == ::connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        std::cerr << "connect server error" << std::endl;
        close(clientfd); // 连接失败就关闭描述符
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    ::sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd); // pthread_create;
    readTask.detach();                               // pthread_detach;

    //无限循环， main 线程用于接收用户输入，负责发送数据
    while (1)
    {
        //显示首页面菜单 登录、注册、退出
        std::cout << "========================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << "choice: ";

        int choice = 0;
        std::cin >> choice; // 输入选择，整数，我们在输入的时候有加回车
        std::cin.get();     // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login 登录业务
        {
            int id = 0;
            char pwd[50] = {0};
            std::cout << "userid: ";
            std::cin >> id;
            std::cin.get(); // 读掉缓冲区残留的回车
            std::cout << "userpassword: ";
            std::cin.getline(pwd, 50);

            // 客户端和服务器要保持语言一致！！！
            nlohmann::json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            std::string request = js.dump();

            g_isLoginSuccess = false;

            int len = ::send(clientfd, request.c_str(), ::strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                std::cerr << "send login msg error: " << request << std::endl;
            }

            ::sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里

            if (g_isLoginSuccess) // 登录成功了
            {
                // 进入聊天主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: // register 注册业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            std::cout << "username: ";
            std::cin.getline(name, 50); // cin() 有回车才结束，遇见空格就结束一个参数的输入
            std::cout << "userpassword: ";
            std::cin.getline(pwd, 50);

            nlohmann::json js;
            js["msgid"] = REGIST_MSG;
            js["name"] = name;
            js["password"] = pwd;
            std::string request = js.dump();

            int len = ::send(clientfd, request.c_str(), ::strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                std::cerr << "send reg msg error: " << request << std::endl;
            }

            ::sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3: // quit 退出业务，因为还没登录，就退出
        {
            ::close(clientfd); // 关闭文件描述符
            ::sem_destroy(&rwsem);
            ::exit(0);
        }
        break;
        default:
            std::cerr << "invalid input!" << std::endl;
            break;
        }
    }

    return 0;
}

// 处理注册的响应逻辑
void doRegistResponse(nlohmann::json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) //注册失败
    {
        std::cerr << "name is already exist, register error!" << std::endl;
    }
    else // 注册成功
    {
        std::cout << "name register success, userid is "
                  << responsejs["id"]
                  << ", do not forget it!"
                  << std::endl;
    }
}

// 处理登录的响应逻辑
void doLoginResponse(nlohmann::json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 登录失败
    {
        std::cerr << responsejs["errmsg"] << std::endl;
        g_isLoginSuccess = false;
    }
    else // 登录成功
    {
        // 记录当前用户的 id 和 name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);
        // std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
        // std::cout << responsejs.dump() << std::endl;
        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            //初始化
            g_currentUserFriendList.clear();
            // 重新装入，新的登录
            std::vector<std::string> vec = responsejs["friends"];
            for (auto &str : vec)
            {
                nlohmann::json js = nlohmann::json::parse(str); // 解析
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();
            std::vector<std::string> vec1 = responsejs["groups"];
            for (auto &groupstr : vec1)
            {
                nlohmann::json grpjs = nlohmann::json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                std::vector<std::string> vec2 = grpjs["users"];

                for (auto &userstr : vec2)
                {
                    nlohmann::json js = nlohmann::json::parse(userstr); // 解析
                    GroupUser user;
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }
        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息  个人聊天信息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            std::vector<std::string> vec = responsejs["offlinemsg"];
            for (std::string &str : vec)
            {
                nlohmann::json js = nlohmann::json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    std::cout << js["time"].get<std::string>()
                              << " ["
                              << js["id"]
                              << "]"
                              << js["name"].get<std::string>()
                              << " said: "
                              << js["msg"].get<std::string>()
                              << std::endl;
                }
                else
                {
                    std::cout << "Group Message["
                              << js["groupid"]
                              << "]:"
                              << js["time"].get<std::string>()
                              << " ["
                              << js["id"]
                              << "]"
                              << js["name"].get<std::string>()
                              << " said: "
                              << js["msg"].get<std::string>()
                              << std::endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}

// 登录成功后，启动子线程 - 接收线程
void readTaskHandler(int clientfd)
{
    while (1)
    {
        char buffer[1024] = {0};
        int len = ::recv(clientfd, buffer, 1024, 0); //阻塞
        if (-1 == len || 0 == len)
        {
            ::close(clientfd);
            ::exit(-1);
        }

        // 接收 ChatServer 转发的数据，反序列化生成 json 数据对象
        nlohmann::json js = nlohmann::json::parse(buffer);
        int msgtype = js["msgid"].get<int>();

        if (ONE_CHAT_MSG == msgtype)
        {
            std::cout << js["time"].get<std::string>()
                      << " ["
                      << js["id"]
                      << "]"
                      << js["name"].get<std::string>()
                      << " said: "
                      << js["msg"].get<std::string>()
                      << std::endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype) // 如果是群组消息
        {
            std::cout << "Group Message["
                      << js["groupid"]
                      << "]:"
                      << js["time"].get<std::string>()
                      << " ["
                      << js["id"]
                      << "]"
                      << js["name"].get<std::string>()
                      << " said: "
                      << js["msg"].get<std::string>()
                      << std::endl;
            continue;
        }

        if (LOGIN_MSG_ACK == msgtype) // 登录响应消息
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            ::sem_post(&rwsem);  // 通知主线程，登录结果处理完成
            continue;
        }

        if (REGIST_MSG_ACK == msgtype)
        {
            doRegistResponse(js);
            ::sem_post(&rwsem); // 通知主线程，注册结果处理完成
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    std::cout << "======================login user======================" << std::endl;
    std::cout << "current login user => id:"
              << g_currentUser.getId()
              << " name:"
              << g_currentUser.getName()
              << std::endl;
    std::cout << "----------------------friend list---------------------" << std::endl;
    if (!g_currentUserFriendList.empty()) // 如果好友列表不为空
    {
        for (auto &user : g_currentUserFriendList)
        {
            std::cout << user.getId()
                      << " "
                      << user.getName()
                      << " "
                      << user.getState()
                      << std::endl;
        }
    }

    std::cout << "----------------------group list----------------------" << std::endl;
    if (!g_currentUserGroupList.empty()) // 群组信息不为空，也打印出来
    {
        for (auto &group : g_currentUserGroupList)
        {
            std::cout << group.getId()
                      << " "
                      << group.getName()
                      << " "
                      << group.getDesc()
                      << std::endl;

            for (auto &user : group.getUsers())
            {
                std::cout << user.getId()
                          << " "
                          << user.getName()
                          << " "
                          << user.getState()
                          << " "
                          << user.getRole()
                          << std::endl;
            }
        }
    }
    std::cout << "======================================================" << std::endl;
}

// 命令处理方法的声明 ，传递 client 的 fd，用户输入的数据 string
// "help" command handler
void help(int fd = 0, std::string str = "");
// "chat" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "deletefriend" command handler
void deletefriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "exitgroup" command handler
void exitgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "loginout" command handler
void loginout(int, std::string);

// 系统支持的客户端命令列表，clientmap
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "Show all supported commands, format:'help'"},
    {"chat", "One to one chat, format:'chat:friendid:message'"},
    {"addfriend", "Add friend, format:'addfriend:friendid'"},
    {"deletefriend", "Delete friend, format:'deletefriend:friendid'"},
    {"creategroup", "Create group, format:'creategroup:groupname:groupdesc'"},
    {"addgroup", "Add group, format:'addgroup:groupid'"},
    {"exitgroup", "Exit group, format:'exitgroup:groupid'"},
    {"groupchat", "Group chat, format:'groupchat:groupid:message'"},
    {"loginout", "Login out, format:'loginout'"}};

// 注册系统支持的客户端命令处理, function 函数指针函数对象类型 ，如果发现命令就执行相应的方法
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"deletefriend", deletefriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"exitgroup", exitgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序，先显示一下系统支持的命令
void mainMenu(int clientfd)
{
    help(); // 参数有默认值

    char buffer[1024] = {0};
    while (isMainMenuRunning) // 不断的输出
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command; // 存储命令

        int idx = commandbuf.find(":"); // 找到了返回字符的起始下标
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx); // 把命令取出来
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }
        // second 命令名字对应的处理函数
        // 调用相应命令的事件处理回调，mainMenu 对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void help(int, std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap)
    {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

// "chat" command handler
void chat(int clientfd, std::string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int friendid = std::atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    nlohmann::json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}

// "addfriend" command handler
void addfriend(int clientfd, std::string str)
{
    int friendid = std::atoi(str.c_str()); // friendid 转为整数
    nlohmann::json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump(); // 序列化

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
}

// "deletefriend" command handler
void deletefriend(int clientfd, std::string str)
{
    int friendid = std::atoi(str.c_str()); // friendid 转为整数
    nlohmann::json js;
    js["msgid"] = DELETE_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump(); // 序列化

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send deletefriend msg error -> " << buffer << std::endl;
    }
}

// "creategroup" command handler
void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":"); // groupname:groupdesc
    if (-1 == idx)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);                    // 获取群名
    std::string groupdesc = str.substr(idx + 1, str.size() - idx); // 获取群描述

    nlohmann::json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump(); // json 序列化成字符串

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}

// "addgroup" command handler
void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str()); // groupid 转成整数
    nlohmann::json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump(); // json 序列化成字符串

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}

// "exitgroup" command handler
void exitgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str()); // groupid 转成整数
    nlohmann::json js;
    js["msgid"] = EXIT_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump(); // json 序列化成字符串

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send exitgroup msg error -> " << buffer << std::endl;
    }
}

// "groupchat" command handler
void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":"); // groupid:message
    if (-1 == idx)
    {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    nlohmann::json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
}

// "loginout" command handler
void loginout(int clientfd, std::string str)
{
    nlohmann::json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), ::strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send loginout msg error -> " << buffer << std::endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = ::localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);

    return std::string(date);
}
