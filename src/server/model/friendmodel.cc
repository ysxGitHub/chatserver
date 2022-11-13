#include "friendmodel.hpp"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     mysql.update(sql);
    // }

    auto mysql = _pool->getMySQLConnection();
    mysql->update(sql);
}

// 删除好友关系
void FriendModel::del(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from friend where userid=%d and friendid=%d", userid, friendid);

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     mysql.update(sql);
    // }

    auto mysql = _pool->getMySQLConnection();
    mysql->update(sql);
}

// 返回用户好友列表
std::vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userid);

    std::vector<User> vec;

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     MYSQL_RES *res = mysql.query(sql);
    //     if (res != nullptr)
    //     {
    //         // 把 userid 的好友请求的用户信息都存入数组中
    //         MYSQL_ROW row;
    //         while ((row = mysql_fetch_row(res)) != nullptr)
    //         {
    //             User user;
    //             user.setId(std::atoi(row[0]));
    //             user.setName(row[1]);
    //             user.setState(row[2]);
    //             vec.push_back(user);
    //         }
    //         mysql_free_result(res);
    //         return vec;
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    MYSQL_RES *res = mysql->query(sql);
    if(res != nullptr)
    {
        //把userid的好友请求的用户信息都存入数组中
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            User user;
            user.setId(std::atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.push_back(user);
        }
        mysql_free_result(res);
        return vec;
    }

    return vec;
}

