#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <fcntl.h>
#include "server.h"
#include "eventloop.h"
#include "acceptor.h"
#include "postman.h"

Server *Server::global_server = new Server();  // Server的单例

Server *Server::getInstance()
{
    assert(global_server);
    return global_server;
}

Server::Server()
{
    _loop = new EventLoop();
    _acceptor = new Acceptor();
    if(_loop == nullptr || _acceptor == nullptr)
    {
        FATAL_ERROR("init Server error");
    }
    _status = 0; // ready to run;
}

Server::~Server()
{
    _loop->stop();
    delete _loop;
    _acceptor->stop();
    delete _acceptor;
    for (auto &i : _watchers)
    {
        ::close(i.first);
        delete i.second;
    }
    for(auto &i: _idle_postmans)
    {
        delete i;
    }
}

int Server::run(int port)
{
    if (E_OK != _acceptor->start(port))
    {
        FATAL_ERROR("init Server error");
    }
    _status = 1; // ready to loop;

    // add _accpetor into loop
    _acceptor->setLoop(_loop);
    _acceptor->addEvent(EPOLLIN);

    while (_status == 1)
    {
        _loop->loop(1); // loop一下就停止
    }
    return E_OK;
}

void Server::stop()
{
    _loop->stop();
    _acceptor->stop();
    _status = -1; // stopped
}

void Server::newConnectionCallback(WATCHER_TYPE t, int fd, sockaddr_in& addr)
{
    // TODO: 选取一个loop
    Postman *p = newPostman(t, fd, _loop);
    p->host = addr.sin_addr.s_addr;
    p->port = addr.sin_port;
    p->addEvent(EPOLLIN);
}

Postman *Server::newPostman(WATCHER_TYPE type, int fd, EventLoop *loop)
{
    if(fd <= 0 || loop == nullptr)
        return nullptr;

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
// add、upd、delwatcher都分为两步：更新_watchers、更新相应的loop
int Server::addWatcher(IOWatcher *w)
{
    int fd = w->getFd();
    assert(_watchers.find(fd) == _watchers.end());
    // 设置fd非阻塞
    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags|O_NONBLOCK|O_CLOEXEC);
    
    _watchers[fd] = w;

    return _loop->registerWatcher(w);
}

int Server::updWatcher(IOWatcher *w)
{
    auto w_iter = _watchers.find(w->getFd());
    if (w_iter == _watchers.end())
    {
        return addWatcher(w);
    }
    IOWatcher *watcher = w_iter->second;
    assert(watcher == w);

    _loop->updateWatcher(w);
    return E_OK;
}

int Server::delWatcher(IOWatcher *w)
{
    if(!w) return E_CANCEL;
    auto w_iter = _watchers.find(w->getFd());
    if (w_iter == _watchers.end())
    {
        return E_CANCEL;
    }

    IOWatcher *watcher = w_iter->second;
    assert(watcher == w);
    _loop->unRegisterWatcher(w);
    ::close(w->getFd());

    if(w->getType() == LOCAL_POSTMAN)
    {
        Postman *p = dynamic_cast<Postman*>(w);
        if(p->peer_postman) delWatcher(p->peer_postman);
        delPostman(p);
    }
    else if(w->getType() == REMOTE_POSTMAN)
    {
        delPostman(dynamic_cast<Postman *>(w));
    }
    
    _watchers.erase(w_iter);
    return E_OK;
}
