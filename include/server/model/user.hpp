#ifndef USER_H
#define USER_H

#include <string>

#include "mysqlconnectionpool.hpp"

// User 表的 ORM 类
class User
{
public:
    User(int id = -1,
         std::string name = "",
         std::string pwd = "",
         std::string state = "offline")
    {
        id_ = id;
        name_ = name;
        password_ = pwd;
        state_ = state;
    }

    // 设置
    void setId(int id) { id_ = id; }
    void setName(std::string name) { name_ = name; }
    void setPwd(std::string pwd) { password_ = pwd; }
    void setState(std::string state) { state_ = state; }

    // 获取
    int getId() const { return id_; }
    std::string getName() const { return name_; }
    std::string getPwd() const { return password_; }
    std::string getState() { return state_; }

protected:
    int id_;               // ID 号
    std::string name_;     // 姓名
    std::string password_; // 密码
    std::string state_;    // 状态
};

#endif
