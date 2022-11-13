#include <fstream>
#include <thread>
#include <iostream>

#include "nlohmann/json.hpp"
#include "mysqlconnectionpool.hpp"


MySQLConnectionPool *MySQLConnectionPool::getMySQLConnectionPool()
{
    // 创建静态局部对象, C++11 不用考虑线程安全
	// 在当前线程执行到需要初始化变量的地方时，如果有其他线程正在初始化该变量，则阻塞当前线程，直到初始化完成为止。
    static MySQLConnectionPool pool;
    return &pool;
}

bool MySQLConnectionPool::parseJsonFile()
{
    std::ifstream ifs("../json/dbconf.json"); // 得到流对象
    if(!ifs.is_open())
    {
        std::cerr << "dbconf.json file is not exist!" << std::endl;
        return false;
    }

    nlohmann::json js; // 以文件流形式读取 json 文件
    ifs >> js;

    if(js.contains("userName"))
        _user = js.at("userName");
    else
        _user = "root";

    if(js.contains("password"))
        _passwd = js.at("password");
    else
        _passwd = "123456";

    if(js.contains("dbName"))
        _dbName = js.at("dbName");
    else
        _dbName = "chat";

    if(js.contains("ip"))
        _ip = js.at("ip");
    else
        _ip = "127.0.0.1";

    if(js.contains("port"))
        _port = js.at("port");
    else
        _port = 3306;

    if(js.contains("minSize"))
        _minSize = js.at("minSize");
    else
        _minSize = 10;

    if(js.contains("maxSize"))
        _maxSize = js.at("maxSize");
    else
        _maxSize = 50;

    if(js.contains("timeout"))
        _timeout = js.at("timeout");
    else
        _timeout = 1000;

	if(js.contains("maxIdleTime"))
        _maxIdleTime = js.at("maxIdleTime");
    else
        _maxIdleTime = 5000;

    ifs.close();

    return true;
}

MySQLConnectionPool::MySQLConnectionPool()
{
    if(!parseJsonFile())
    {
        return ;
    }
    for(int i = 0; i < _minSize; ++i)
    {
        addConnection();
    }
    // 生产连接池中的连接 任务函数 = [有名函数，匿名函数，类的静态成员函数，类的非静态成员函数，可调用对象]
    std::thread producer(&MySQLConnectionPool::produceConnection, this);
    producer.detach();  // 主线程还要做其他事情（得到连接），所以不能阻塞主线程

    // 检测有么有要销毁的连接
    std::thread recycler(&MySQLConnectionPool::recycleConnection, this);
    recycler.detach();
}

void MySQLConnectionPool::addConnection()
{
    MySQLConn *conn = new MySQLConn;
    // 进行数据库连接
    conn->connect(_user, _passwd, _dbName, _ip, _port);
    // 更新时间点
    conn->refreshAliveTime();
    // 该连接加入队列
    _connectionQ.push(conn);
}

std::shared_ptr<MySQLConn> MySQLConnectionPool::getMySQLConnection()
{
    // 消费者线程
    std::unique_lock<std::mutex> locker(_mutexQ);
    // 先判断连接池是否有连接可用
    while(_connectionQ.empty())
    {
		// 阻塞一定时间 m_timeout, 阻塞返回 timeout 则队列还是为空
        if(std::cv_status::timeout == _ccond.wait_for(locker, std::chrono::milliseconds(_timeout)))
        {
            if(_connectionQ.empty())
            {
                continue;
            }
        }
    }
	// 连接池不为空，返回头部连接并且要弹出
	// 利用共享智能指针可以进行 连接的指针回收，这里需要指定第二个参数
    std::shared_ptr<MySQLConn> connptr(_connectionQ.front(),
        [this](MySQLConn *conn)
        {
            std::lock_guard<std::mutex> locker(_mutexQ); // 自动加锁与解锁，但是不能控制锁定的范围
            conn->refreshAliveTime();  // 更新时间节点
            _connectionQ.push(conn);   // 加入连接池队列
        });

    _connectionQ.pop();
    _pcond.notify_all();

    return connptr;
}

void MySQLConnectionPool::produceConnection()
{
    //生产者线程
    while(true)
    {
        std::unique_lock<std::mutex> locker(_mutexQ);
        while(_connectionQ.size() >= _minSize)
        {
            _pcond.wait(locker); // 连接数够时，多个生产者线程都会阻塞这里
        }
        addConnection();      // 添加连接
        _ccond.notify_all();  // 唤醒消费者
    }
}

void MySQLConnectionPool::recycleConnection()
{
    // 销毁者线程
    while(true)
    {  // 500ms 周期性检测
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::lock_guard<std::mutex> locker(_mutexQ);
        while(_connectionQ.size() > _minSize)
        {   // 队头一定是存活最久的连接
            auto conn = _connectionQ.front();
            if(conn->getAliveTime() >= _maxIdleTime)
            {
                _connectionQ.pop();
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}

MySQLConnectionPool::~MySQLConnectionPool()
{
    while(!_connectionQ.empty())
    {
        auto conn = _connectionQ.front();
        _connectionQ.pop();
        delete conn;
    }
}

