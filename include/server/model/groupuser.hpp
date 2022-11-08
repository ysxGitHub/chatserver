#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

//群组用户，多了一个role角色信息（创建者还是成员），从User类直接继承，复用User的其它信息

class GroupUser : public User
{
public:
    void setRole(std::string role) { _role = role; }
    std::string getRole() const { return _role; }

private:
    std::string _role;
};


#endif