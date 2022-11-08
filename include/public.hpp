#ifndef PUBLIC_H
#define PUBLIC_H

/**
 * server和client的公共文件
 **/
enum EnMsgType
{
    LOGIN_MSG = 1,   // 登录消息
    LOGIN_MSG_ACK,   // 登录响应消息
    LOGINOUT_MSG,    // 下线消息
    REGIST_MSG,      // 注册消息
    REGIST_MSG_ACK,  // 注册响应消息
    ONE_CHAT_MSG,    // 聊天消息    向对端发送消息
    // 消息格式大概为： {"msgid":xx, "id":xx "name":"zhang san", "to":xx, "msg":"xx"}

    ADD_FRIEND_MSG,  // 添加好友
    CREATE_GROUP_MSG,// 创建组群
    ADD_GROUP_MSG,   // 加入组群
    GROUP_CHAT_MSG,  // 群聊天
};

#endif

// {"msgid":4, "name":"llb", "password":"111"}

// {"msgid":1, "id":23, "password":"123456"}

// {"msgid":1, "id":24, "password":"111"}

// {"msgid":6,"id":23,"from":"ysx","toid":24,"msg":"hahaha!"}

// {"msgid":7,"id":23,"friendid":24}
