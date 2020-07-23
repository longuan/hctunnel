#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <fcntl.h>
#include "server.h"
#include "eventloop.h"
#include "acceptor.h"
#include "postman.h"
#include "threadpool.h"

Server *Server::_global_server = new Server();  // Server的单例

Server::Server():_last_count(std::pair<in_addr_t, int>(0,0))
{
    _acceptor = new Acceptor();
    _main_loop = new EventLoop(_acceptor);
    _io_loops = new ThreadPool();
}

Server::~Server()
{
    // 先删除EventLoop，再删除watchers
    delete _main_loop;
    delete _io_loops;
    delete _acceptor;
}

int Server::start(int port)
{
    if (E_OK != _acceptor->start(port))
    {
        FATAL_ERROR("init Server error");
    }

    _acceptor->addEvent(EPOLLIN);
    std::cout << "[*] main loop thread ID is: " << _main_loop->getThreadID() << std::endl;
    _main_loop->loop();
    return E_OK;
}

void Server::newConnectionCallback(WATCHER_TYPE t, int fd, sockaddr_in& addr)
{
    // if(addr.sin_addr.s_addr == _last_count.first)
    // {
    //     if(_last_count.second >= 30)
    //     {
    //         ::close(fd);
    //         std::cout << "connection refuse" << std::endl;
    //         return;
    //     }
    // }
    // else
    // {
    //     _last_count.first = addr.sin_addr.s_addr;
    //     _last_count.second = 0;
    // }
    // _last_count.second++;

    EventLoop *loop = _io_loops->getNextLoop();
    Postman *p = loop->newPostman(t, fd);
    assert(p);
    p->host = addr.sin_addr.s_addr;
    p->port = addr.sin_port;
    p->updateLastTime();
    p->setEvents(EPOLLIN);

    loop->addFd(p);
}
