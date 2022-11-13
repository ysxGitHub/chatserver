#include "InetAddress.h"

#include <string.h>
#include <strings.h>
#include <stdio.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    ::bzero(&addr_, sizeof(addr_)); // 清零
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port); // 主机字节序转网络字节序
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)); // 读出整数的表示网络字节序转成本地字节序
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ::ntohs(addr_.sin_port); // 网络字节序转为主机字节序
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)); // 将大端的整形数, 转换为小端的点分十进制的IP地址
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    ::sprintf(buf + end, ":%u", port);
    return buf;
}

#if 0
#include <iostream>
int main()
{
    InetAddress addr(8888);
    std::cout<<addr.toIpPort()<<std::endl;
    return 0;
}
#endif
