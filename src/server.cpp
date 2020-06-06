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
    _main_loop = new EventLoop();
    _acceptor = new Acceptor();
    _io_loops = new ThreadPool();
}

Server::~Server()
{
    // 先删除EventLoop，再删除watchers
    delete _main_loop;
    delete _io_loops;
    delete _acceptor;

    for (auto &i : watchers)
    {
        ::close(i.first);
        delete i.second;
    }
    for(auto &i: _idle_postmans)
    {
        delete i;
    }
}

int Server::start(int port)
{
    // add _accpetor into loop
    _acceptor->setLoop(_main_loop);

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
    if(addr.sin_addr.s_addr == _last_count.first)
    {
        if(_last_count.second >= 30)
        {
            ::close(fd);
            std::cout << "connection refuse" << std::endl;
            return;
        }
    }
    else
    {
        _last_count.first = addr.sin_addr.s_addr;
        _last_count.second = 0;
    }
    _last_count.second++;

    EventLoop *loop = _io_loops->getNextLoop();
    Postman *p = newPostman(t, fd, loop);
    assert(p);
    p->setStatus(CONNECTED);
    p->host = addr.sin_addr.s_addr;
    p->port = addr.sin_port;
    p->addEvent(EPOLLIN);
    p->updateLastTime();
}

Postman *Server::newPostman(WATCHER_TYPE type, int fd, EventLoop *loop)
{
    if(fd <= 0 || loop == nullptr)
        return nullptr;

    std::lock_guard lg(_mutex);
    if(_idle_postmans.empty())
    {
        Postman *p = new Postman(type);
        p->setFd(fd);
        p->setLoop(loop);
        return p;
    }
    else
    {
        Postman *p = _idle_postmans.back();
        _idle_postmans.pop_back();
        p->setType(type);
        p->setFd(fd);
        p->setLoop(loop);
        return p;
    }
}

void Server::delPostman(Postman *p)
{
    if(!p) return;
    std::lock_guard lg(_mutex);
    if (_last_count.first == p->host)
        _last_count.second--;
    if (_idle_postmans.size() < 64)
    {
        p->clear();
        _idle_postmans.push_back(p);
    }
    else
    {
        delete p;
    }
}

// TODO: 每次addwatcher、updWatcher、delWatcher都会调用一次epoll_ctl，会不会影响性能
// add、upd、delwatcher都分为两步：更新watchers、更新相应的loop
int Server::addWatcher(IOWatcher *w)
{
    int fd = w->getFd();
    assert(watchers.find(fd) == watchers.end());
    // 设置fd非阻塞
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags|O_NONBLOCK|O_CLOEXEC);
    {
        std::lock_guard lg(_mutex);
        watchers[fd] = w;
    }

    return w->getLoop()->registerWatcher(w);
}

int Server::updWatcher(IOWatcher *w)
{
    std::unique_lock ul(_mutex);
    auto w_iter = watchers.find(w->getFd());
    if (w_iter == watchers.end())
    {
        ul.unlock();
        return addWatcher(w);
    }
    IOWatcher *watcher = w_iter->second;
    ul.unlock();
    assert(watcher == w);

    w->getLoop()->updateWatcher(w);
    return E_OK;
}

int Server::delWatcher(IOWatcher *w)
{
    if(!w) return E_CANCEL;
    decltype(watchers)::iterator w_iter;
    {
        std::lock_guard lg(_mutex);
        w_iter = watchers.find(w->getFd());
        if (w_iter == watchers.end())
        {
            return E_CANCEL;
        }
    }
    IOWatcher *watcher = w_iter->second;
    assert(watcher == w);
    w->getLoop()->unRegisterWatcher(w);
    ::close(w->getFd());

    if(w->getType() == LOCAL_POSTMAN)
    {
        Postman *p = dynamic_cast<Postman*>(w);
        if(p->peer_postman) delWatcher(p->peer_postman);
        delPostman(p);
    }
    else if(w->getType() == REMOTE_POSTMAN)
    {
        Postman *p = dynamic_cast<Postman *>(w);
        delPostman(p);
    }
    
    {
        std::lock_guard lg(_mutex);
        watchers.erase(w_iter);
    }
    return E_OK;
}