#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <unistd.h>
#include "server.h"
#include "eventloop.h"
#include "acceptor.h"
#include "utils.h"

int Acceptor::start(int port)
{
    int reuse = 1;
    ::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(int));

    sockaddr_in bind_addr;
    ::memset(&bind_addr, 0, sizeof(sockaddr_in));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(port);
    int bind_result = ::bind(_fd, (sockaddr *)&bind_addr, sizeof(sockaddr));
    if (bind_result < 0)
        return E_FATAL;
    _listenport = port;
    if (::listen(_fd, SOMAXCONN) == -1)
        return E_FATAL;
    std::cout << "[" << std::this_thread::get_id();
    std::cout << "][" << _loop->getThreadID();
    std::cout << "] listening "
              << ":" << _listenport << std::endl;
    return E_OK;
}

int Acceptor::acceptClient()
{
    sockaddr_in client_addr;
    ::memset(&client_addr, 0, sizeof(sockaddr_in));
    socklen_t addr_len = sizeof(sockaddr_in);
    int client_fd = ::accept(_fd, (sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("accetp error");
        return E_ERROR;
    }
    std::cout << "[" << std::this_thread::get_id();
    std::cout << "][" << _loop->getThreadID();
    std::cout << "] accept a new client: " << inet_ntoa(client_addr.sin_addr)
              << ":" << client_addr.sin_port << " with fd " << client_fd
              << std::endl;

    Server *s = Server::getInstance();
    s->newConnectionCallback(LOCAL_POSTMAN, client_fd, client_addr);
    return E_OK;
}

void Acceptor::handleEvent(EVENT_TYPE revents)
{
    if ((revents & _events) == 0)
    {
        return;
    }

    if (revents == EPOLLIN)
    {
        acceptClient();
    }
}

// TODO: 待完善
void Acceptor::handleClose()
{
    _loop->deleteFd(_fd);
}