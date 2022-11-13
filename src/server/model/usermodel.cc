#include "usermodel.hpp"

// User 表的增加方法
bool UserModel::insert(User &user)
{
    // 1.组装 sql 语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // MySQLConn mysql; //定义一个 mysql 对象
    // if (mysql.connect())
    // {
    //     if (mysql.update(sql)) //更新这个sql语句传进去
    //     {
    //         // 获取插入成功的用户数据生成的主键 id , 拿到数据库生成的 id 作为用户的 id 号
    //         user.setId(mysql_insert_id(mysql.getConnection()));
    //         return true;
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    if(mysql->update(sql)) //更新这个 sql 语句传进去
    {
        // 获取插入成功的用户数据生成的主键 id, 拿到数据库生成的 id 作为用户的 id 号
        user.setId(mysql_insert_id(mysql->getConnection()));
        return true;
    }

    return false;
}

// 根据用户号码查询用户信息

User UserModel::query(int id)
{
    // 1.组装 sql 语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    // MySQLConn mysql; // 创建 mysql 对象
    // if (mysql.connect())
    // {
    //     MYSQL_RES *res = mysql.query(sql); // 调用 mysql 数据库的查询
    //     if (res != nullptr)
    //     {
    //         MYSQL_ROW row = mysql_fetch_row(res);
    //         if (row != nullptr)
    //         {
    //             User user;
    //             user.setId(std::atoi(row[0]));
    //             user.setName(row[1]);
    //             user.setPwd(row[2]);
    //             user.setState(row[3]);
    //             mysql_free_result(res); // 释放结果集
    //             return user;
    //         }
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    MYSQL_RES *res = mysql->query(sql); // 调用 mysql 数据库的查询
    if(res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if(row != nullptr)
        {
            User user;
            user.setId(std::atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res); // 释放结果集
            return user;
        }
    }

    return User();
}

// 更新用户的状态信息
bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     if (mysql.update(sql))
    //     {
    //         return true;
    //     }
    // }

    auto mysql = _pool->getMySQLConnection();
    if(mysql->update(sql))
    {
        return true;
    }

    return false;
}

// 重置用户的状态信息
void UserModel::resetState()
{
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    // MySQLConn mysql;
    // if (mysql.connect())
    // {
    //     mysql.update(sql);
    // }

    auto mysql = _pool->getMySQLConnection();
    mysql->update(sql);
}
