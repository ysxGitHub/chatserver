#ifndef DB_H
#define DB_H

#include <string>
#include <chrono>
#include "mysql/mysql.h"

//数据库配置信息
// static std::string ip = "127.0.0.1";
// static unsigned short port = 3306;
// static std::string user = "root";
// static std::string password = "123456";
// static std::string dbname = "chat";

class MySQLConn
{
public:
	// 初始化数据库连接
	MySQLConn();
	// 释放数据库连接
	~MySQLConn();
	// 连接数据库
	bool connect(std::string user = "root",
                 std::string password = "123456",
                 std::string dbname = "chat",
                 std::string ip = "127.0.0.1",
                 unsigned short port = 3306);
	// 更新数据库：insert，update，delete
	bool update(std::string sql);
	// 查询数据库
	MYSQL_RES * query(std::string sql);
	// // 遍历查询得到的结果集
	// bool next();
	// // 得到结果集中的字段值
	// std::string value(int index);
	// 事务操作
	bool transaction();
	// 提交事务
	bool commit();
	// 事务回滚
	bool rollback();
    // 获取连接
    MYSQL *getConnection() const ;

	// 刷新起始的空闲时间点
	void refreshAliveTime();
	// 计算连接存活的总时长
	long long getAliveTime();

private:
	MYSQL* _conn = nullptr;
	// MYSQL_RES* _result = nullptr;
	// MYSQL_ROW _row = nullptr;
	// void freeResult();

    // 绝对时钟
    std::chrono::steady_clock::time_point _alivetime;
};

#endif