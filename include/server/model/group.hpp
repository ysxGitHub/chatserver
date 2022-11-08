#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"

#include <vector>
#include <string>

//User表的ORM类
class Group
{
public:
    Group(int id = -1, std::string name = "", std::string desc = "")
    {
        _id = id;
        _name = name;
        _desc = desc;
    }

    void setId(int id) { _id = id; }
    void setName(std::string name) { _name = name; }
    void setDesc(std::string desc) { _desc = desc; }

    int getId() const { return _id; }
    std::string getName() const { return _name; }
    std::string getDesc() const { return _desc; }

    std::vector<GroupUser> &getUsers() { return _users; }

private:
    int _id;
    std::string _name;              //组的名称
    std::string _desc;              //组的功能描述
    std::vector<GroupUser> _users;  //组的成员
};



#endif