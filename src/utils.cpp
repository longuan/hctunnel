
#include <cstdlib>   // exit()
#include <cstdio>    // perror()
#include <unistd.h>
#include <iostream>
#include <netdb.h>
#include <cstring>
#include <sys/ioctl.h>
#include "utils.h"

void FATAL_ERROR(const char *msg)
{
    std::cout << msg << std::endl;
    ::perror(NULL);
    ::exit(E_FATAL);
}

ssize_t readall(int fd, std::string &inBuffer)
{
    size_t length = 0;
    if (::ioctl(fd, FIONREAD, &length) < 0)
    {
        std::cout << "[!] readall - ioctl error" << std::endl;
        return -1;
    }

    if (length <= 0)
    {
        std::cout << "[!] readAll error: got length -- " << length << std::endl;
        return 0;
    }
    inBuffer.resize(length);

    int nread = 0;
    if ((nread = read(fd, inBuffer.data(), length)) < 0)
    { 
        std::cout << "[!] read error" << std::endl;
        // if (errno == EINTR || errno == EAGAIN)
        return -1;
    }
    else if(nread == 0)
    {
        return 0;
    }
    else
    {
        return nread==length?length:0;
    }
}

ssize_t writeall(int fd, std::string_view outBuffer)
{
    size_t nleft = outBuffer.size();
    ssize_t nwrite = 0;
    ssize_t writeSum = 0;
    const char *data_ptr = outBuffer.data();
    while(nleft > 0){
        if ((nwrite = write(fd, data_ptr, nleft)) <= 0)
        {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN)
            {
                return writeSum;
            }
            else
            {
                perror("write error");
                return -1;
            }
        }
        writeSum += nwrite;
        nleft -= nwrite;
        data_ptr += nwrite;
    }
    return writeSum;
}

bool startswith(const std::string &buf, const std::string &prefix)
{
    if(!buf.compare(0, prefix.size(), prefix))
    {
        return true;
    }
    else
        return false;
}

bool startswith(const std::string_view buf, const std::string &prefix)
{
    if (!buf.compare(0, prefix.size(), prefix))
    {
        return true;
    }
    else
        return false;
}

bool endswith(const std::string_view buf, const std::string &suffix)
{
    if (!buf.compare(buf.size()-suffix.size(), suffix.size(), suffix))
    {
        return true;
    }
    else
        return false;
}

std::string_view headerValue(std::string_view msg_header, const std::string key)
{
    auto key_idx = msg_header.find(key);
    if(key_idx == std::string_view::npos)
        return msg_header.substr(0,0);
    else
    {
        auto eol_idx = msg_header.find("\r\n", key_idx);
        auto value_start = key_idx + key.size();
        return msg_header.substr(value_start, eol_idx -value_start);
    }
}

int getHostIp(const char *server_name, std::vector<in_addr> &ips)
{
    addrinfo hints, *res;
    in_addr addr;
    int err;

    ::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    if ((err = getaddrinfo(server_name, NULL, &hints, &res)) != 0)
    {
        return E_ERROR;
    }

    {
        addrinfo *res_temp = res;
        while (res_temp != NULL)
        {
            if (res_temp->ai_family == AF_INET && res_temp->ai_socktype == SOCK_STREAM)
                addr.s_addr = ((sockaddr_in *)(res_temp->ai_addr))->sin_addr.s_addr;
            ips.push_back(addr);
            res_temp = res_temp->ai_next;
        }
    }

    freeaddrinfo(res);
    return E_OK;
}

int connectHost(const in_addr &ip, const int port)
{
    int remote_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (remote_fd < 0)
        return -1;
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    server_addr.sin_addr = ip;
    if (!connect(remote_fd, (sockaddr *)&server_addr, sizeof(server_addr)))
    {
        return remote_fd;
    }
    return -1;
}