#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include <string>
#include <vector>

#include "group.hpp"

//维护群组信息的操作接口方法
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groudid, std::string role);
    // 退出组群
    void exitGroup(int userid, int groudid);

    // 查询用户所在群组信息  在客户端呈现
    std::vector<Group> queryGroups(int userid);
    // 群聊。根据指定的 groupid 查询群组用户 id 列表，除 userid 自己，主要用户群聊业务给群组其它成员群发消息
    std::vector<int> queryGroupUsers(int userid, int groupid);

    MySQLConnectionPool *_pool = MySQLConnectionPool::getMySQLConnectionPool();
};

#endif