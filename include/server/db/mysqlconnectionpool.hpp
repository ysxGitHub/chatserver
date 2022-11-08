#ifndef MYSQLCONNECTIONPOOL
#define MYSQLCONNECTIONPOOL
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>

#include "mysqlconnection.hpp"

class MySQLConnectionPool
{
public:
    // 单例模式<饿汉模式>返回静态局部变量
    static MySQLConnectionPool *getMySQLConnectionPool();
    // 禁止copy构造
    MySQLConnectionPool(const MySQLConnectionPool &obj) = delete;
    // 禁止赋值重载
    MySQLConnectionPool &operator=(const MySQLConnectionPool &obj) = delete;
    // 得到连接池中的连接
    std::shared_ptr<MySQLConn> getMySQLConnection();

    ~MySQLConnectionPool();
private:
    // 单例
    MySQLConnectionPool();
    // 解析json文件
    bool parseJsonFile();
    // 添加连接
    void addConnection();
    // 生成者任务函数
    void produceConnection();
    // 回收者任务函数
    void recycleConnection();

private:
    // 连接池队列
    std::queue<MySQLConn *> _connectionQ;
    // 连接属性
	std::string _ip;
	std::string _user;
	std::string _passwd;
	std::string _dbName;
	unsigned short _port;

    // 连接上下限
    int _minSize;
    int _maxSize;

    // 超时时常，当连接池没有连接时，线程需要等待的时长
    int _timeout;

    // 最大空闲时常，可用的连接太多了，可以销毁一部分
    int _maxIdleTime;

    // 连接池的互斥锁
    std::mutex _mutexQ;
    // 消费者与生产者的条件锁
    std::condition_variable _pcond, _ccond;

    // static MySQLConnectionPool *_pool;
};

#endif


