#include "mysqlconnection.hpp"
#include "Logger.h"

MySQLConn::MySQLConn()
{
    // 初始化连接环境
    _conn = mysql_init(nullptr);
    // 设置编码为 utf-8
    mysql_set_character_set(_conn, "utf8");
}

MySQLConn::~MySQLConn()
{
    if (_conn != nullptr)
    {
        // 释放数据库资源
        mysql_close(_conn);
    }
    // freeResult();
}

bool MySQLConn::connect(std::string user, std::string password, std::string dbname, std::string ip, unsigned short port)
{
    // 2.连接数据库服务器
    MYSQL *ptr = mysql_real_connect(_conn,
                                    ip.c_str(),       // mysql 服务器的主机地址, 写 IP 地址即可 (localhost/NULL 代表本地连接)
                                    user.c_str(),     // 连接 mysql 服务器的用户名, 默认:  root
                                    password.c_str(), // 连接 mysql 服务器用户对应的密码, root 用户的密码
                                    dbname.c_str(),   // 要使用的数据库的名字
                                    port,             // 连接的 mysql 服务器监听的端口, 如果 ==0 , 使用 mysql 的默认端口 3306,  !=0 , 使用指定的这个端口
                                    nullptr,          // 本地套接字, 不使用指定为 NULL
                                    0);

    if (ptr != nullptr)
    {
        LOG_DEBUG("connect mysql success!\n");
    }
    else
    {
        LOG_DEBUG("connect mysql fail!\n");
    }

    return ptr != nullptr;
}

bool MySQLConn::update(std::string sql)
{
    // 更新数据库
    if (mysql_query(_conn, sql.c_str()))
    {

        LOG_DEBUG("%s:%s: ' %s ' 更新失败!\n", __FILE__, __LINE__, sql.c_str());

        return false;
    }
    return true;
}

MYSQL_RES *MySQLConn::query(std::string sql)
{
    // freeResult();
    // 查询数据库
    if (mysql_query(_conn, sql.c_str()))
    {

        LOG_DEBUG("%s:%s: ' %s ' 查询失败!\n", __FILE__, __LINE__, sql.c_str());

        return nullptr;
    }
    // 不调用 mysql_store_result 时结果保存在服务器端，
    // 调用 mysql_store_result 时结果保存在客户端
    // 保存结果集, 一行一行的
    return mysql_store_result(_conn);
}

// bool MySQLConn::next()
// {
// 	if (_result != nullptr) {
// 		// 得到一行信息
// 		_row = mysql_fetch_row(_result);
// 		if (_row != nullptr) {
// 			return true;
// 		}
// 	}
// 	return false;
// }

// // 得到某个字段的值
// std::string MySQLConn::value(int index)
// {
// 	// 得到结果集中的列数（字段）
// 	int colNum = mysql_num_fields(_result);
// 	if (index >= colNum || index < 0) {
// 		return std::string();
// 	}
// 	char* val = _row[index];
// 	// 得到结果集中该字段值的长度
// 	unsigned long length = mysql_fetch_lengths(_result)[index];
// 	return std::string(val, length);
// }

bool MySQLConn::transaction()
{
    // 设置事务为手动提交
    return mysql_autocommit(_conn, false);
}

bool MySQLConn::commit()
{
    // 提交事务
    return mysql_commit(_conn);
}

bool MySQLConn::rollback()
{
    // 事务回滚
    return mysql_rollback(_conn);
}

// void MySQLConn::freeResult()
// {
// 	// 释放资源 - 结果集
// 	if (_result != nullptr) {
// 		mysql_free_result(_result);
// 		_result = nullptr;
// 	}
// }

MYSQL *MySQLConn::getConnection() const
{
    return _conn;
}

// 刷新起始的空闲时间点
void MySQLConn::refreshAliveTime()
{
    _alivetime = std::chrono::steady_clock::now();
}

// 计算连接存活的总时长
long long MySQLConn::getAliveTime()
{
    // 得到时间段
    auto res = std::chrono::steady_clock::now() - _alivetime;
    // 将 ns 转为 ms
    auto millsec = std::chrono::duration_cast<std::chrono::microseconds>(res);
    // 返回多少个 ms
    return millsec.count();
}
