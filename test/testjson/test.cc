#include "nlohmann/json.hpp"

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
using namespace std;
// using namespace nlohmann;


string func1() {
    nlohmann::json js; //定义一个json类型的对象 //添加数组
    js["msg_type"] = 2;
    js["from"] = "li si";
    js["to"] = "zhang san";
    js["msg"] = "hello, are you ok now?";

    // cout<<js<<endl;
    string sendBuf = js.dump(); // json数据对象 =》序列化 json字符串
    // cout<<sendBuf.c_str()<<endl; //转成char*
    return sendBuf;
}

string func2() {
    nlohmann::json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加 key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["li si"] = "hello world";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"li si", "hello world"}};

    // cout<<js<<endl;
    return js.dump();
}

string func3() {
    nlohmann::json js;

    //直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    js["list"] = vec;

    //直接序列化一个map容器
    map<int, string> mmap;
    mmap[1] = "li si";
    mmap[2] = "zhang san";
    mmap[3] = "wang wu";

    js["path"] = mmap;

    // cout<<js<<endl;
    return js.dump();
}

bool parseJsonFile()
{
    std::ifstream ifs("./json/dbconf.json"); // 得到流对象
    if(!ifs.is_open())
    {
        std::cerr << "dbconf.json file is not exist!" << std::endl;
        return false;
    }

    nlohmann::json js; // 以文件流形式读取 json 文件
    ifs >> js;

    if(js.contains("userName"))
        cout << js["userName"] << endl;

    if(js.contains("password"))
        cout << js["password"] << endl;

    if(js.contains("dbName"))
        cout << js["dbName"] << endl;

    if(js.contains("ip"))
        cout << js["ip"] << endl;

    if(js.contains("port"))
        cout << js["port"] << endl;

    if(js.contains("minSize"))
        cout << js["minSize"] << endl;

    if(js.contains("maxSize"))
        cout << js["maxSize"] << endl;

    if(js.contains("timeout"))
        cout << js["timeout"] << endl;
    else
        cout << "000000" << endl;

	if(js.contains("maxIdleTime"))
        cout << js["maxIdleTime"] << endl;

    ifs.close();

    return true;
}


int main()
{
    string recvBuf = func1();

    //数据的反序列化   json字符串 =》反序列化 数据对象（看作容器，方便访问）
    nlohmann::json jsbuf = nlohmann::json::parse(recvBuf);
    cout<<jsbuf["msg_type"]<<endl;
    cout<<jsbuf["from"]<<endl;
    cout<<jsbuf["to"]<<endl;
    cout<<jsbuf["msg"]<<endl;

    cout<<"*********************************"<<endl;
    recvBuf = func2();
    jsbuf = nlohmann::json::parse(recvBuf);
    cout<<jsbuf["id"]<<endl;
    auto vec = jsbuf["id"];
    cout<<vec[0]<<endl;

    auto mmap = jsbuf["msg"];
    cout<<mmap["zhang san"]<<endl;
    cout<<mmap["li si"]<<endl;

    cout<<"*********************************"<<endl;
    recvBuf = func3();
    jsbuf = nlohmann::json::parse(recvBuf);
    vector<int> v = jsbuf["list"]; //js对象里面的数组类型，直接放入vector容器当中
    for (auto val : v)
    {
        cout << val << " ";
    }
    cout << endl;

    cout<<"*********************************"<<endl;
    map<int, string> mymap = jsbuf["path"];
    for (auto &p : mymap)
    {
        cout << p.first << " " << p.second << endl;
    }
    cout << endl;

    parseJsonFile();

    return 0;
}







