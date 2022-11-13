#include "offlinemessagemodel.hpp"

// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, std::string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     mysql.update(sql); // 更新
    // }

    auto mysql = _pool->getMySQLConnection();
    mysql->update(sql); // 更新
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     mysql.update(sql); // 更新
    // }

    auto mysql = _pool->getMySQLConnection();
    mysql->update(sql); // 更新
}

// 查询用户的离线消息
std::vector<std::string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    std::vector<std::string> vec;

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     MYSQL_RES *res = mysql.query(sql);
    //     if (res != nullptr)
    //     {
    //         MYSQL_ROW row;
    //         while ((row = mysql_fetch_row(res)) != nullptr)
    //         {
    //             vec.push_back(row[0]);
    //         }
    //         mysql_free_result(res);
    //         return vec;
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    MYSQL_RES *res = mysql->query(sql);
    if(res != nullptr)
    {
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr)
        {
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
        return vec;
    }

    return vec;
}
