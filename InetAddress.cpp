#include "InetAddress.hpp"
#include <strings.h>
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip) {
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); // 主机字节序 -> 网络字节序
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const {
    char buf[64] = {0};
    // 从网络字节序转换回点分十进制
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort() const {
    // 格式：127.0.0.1:8080
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + strlen(buf), ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}
