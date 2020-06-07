

#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>
#include <netinet/in.h>

void FATAL_ERROR(const char *msg);

enum ERROR_TYPE{
    E_OK = 0,
    E_CANCEL = 3,   // 此次调用取消，提前返回，多用于函数内部检查实参不符合要求
    E_ERROR,        // 执行出错，但是可以继续运行
    E_FATAL,        // 严重出错，程序立即停止运行
};

enum WATCHER_TYPE
{
    LOCAL_POSTMAN = 2,
    REMOTE_POSTMAN,
    ACCEPTOR,
};

ssize_t readall(int fd, std::string &inBuffer);
ssize_t writeall(int fd, std::string_view outBuffer);
bool startswith(const std::string &buf, const std::string &prefix);
bool startswith(const std::string_view buf, const std::string &prefix);
bool endswith(const std::string &buf, const std::string &suffix);
bool endswith(const std::string_view buf, const std::string &suffix);
std::string_view headerValue(std::string_view msg_header, const std::string key);
int getHostIp(const char *server_name, std::vector<in_addr> &ips);
int connectHost(const in_addr& ip, const int port);
int numOfsubstr(const std::string& str, const std::string& sub);

#endif // __UTILS_H__