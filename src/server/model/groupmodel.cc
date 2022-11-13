#include "groupmodel.hpp"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     if (mysql.update(sql))
    //     {
    //         group.setId(mysql_insert_id(mysql.getConnection()));
    //         return true;
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    if(mysql->update(sql))
    {
        group.setId(mysql_insert_id(mysql->getConnection()));
        return true;
    }

    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groudid, std::string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')", groudid, userid, role.c_str());

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     mysql.update(sql);
    // }

    auto mysql = _pool->getMySQLConnection();
    mysql->update(sql);
}

// 退出群聊
void GroupModel::exitGroup(int userid, int groudid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from groupuser where groupid=%d and userid=%d", groudid, userid);

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     mysql.update(sql);
    // }

    auto mysql = _pool->getMySQLConnection();
    mysql->update(sql);
}

// 查询用户所在群组信息  在客户端呈现
std::vector<Group> GroupModel::queryGroups(int userid)
{
    /*
    1. 先根据 userid 在 groupuser 表中查询出该用户所属的群组信息
    2. 在根据群组信息，查询属于该群组的所有用户的 userid ，并且和 user 表进行多表联合查询，查出用户的详细信息
    */
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid=%d", userid);

    //把指定用户的所在的群组信息全部描述出来
    std::vector<Group> groupVec;

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     MYSQL_RES *res = mysql.query(sql);
    //     if (res != nullptr)
    //     {
    //         MYSQL_ROW row;
    //         //查出 userid 所有的群组的信息
    //         while ((row = mysql_fetch_row(res)) != nullptr)
    //         {
    //             Group group;
    //             group.setId(std::stoi(row[0]));
    //             group.setName(row[1]);
    //             group.setDesc(row[2]);
    //             groupVec.push_back(group);
    //         }
    //         mysql_free_result(res);
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    MYSQL_RES *res = mysql->query(sql);
    if(res != nullptr)
    {
        MYSQL_ROW row;
        //查出userid所有的群组的信息
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            Group group;
            group.setId(std::stoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            groupVec.push_back(group);
        }
        mysql_free_result(res);
    }

    // 查询群组的用户信息
    for (auto &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on b.userid = a.id where b.groupid=%d", group.getId());

        MYSQL_RES *res = mysql->query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(std::stoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user); // 存储用户信息
            }
            mysql_free_result(res);
        }
    }

    return groupVec; // 这个东西存着用户的所有群组和所有群组里的用户信息
}

// 群聊。根据指定的 groupid 查询群组用户 id 列表，除 userid 自己，主要用户群聊业务给群组其它成员群发消息
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    std::vector<int> idVec;

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     MYSQL_RES *res = mysql.query(sql);
    //     if (res != nullptr)
    //     {
    //         MYSQL_ROW row;
    //         while ((row = mysql_fetch_row(res)) != nullptr)
    //         {
    //             idVec.push_back(std::atoi(row[0]));
    //         }
    //         mysql_free_result(res);
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    MYSQL_RES *res = mysql->query(sql);
    if(res != nullptr)
    {
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            idVec.push_back(std::atoi(row[0]));
        }
        mysql_free_result(res);
    }

    return idVec;
}
