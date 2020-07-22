#include <iostream>
#include <cassert>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include "postman.h"
#include "eventhandler.h"
#include "eventloop.h"
#include "utils.h"

EventLoop::EventLoop(IOWatcher *initWatcher) : _epollptr(std::make_unique<Epoll>()),
                                                         _stop(true)
{
    _firedEvents.reserve(16);  // 预留空间，为epoll_wait准备
    _thread_id = std::this_thread::get_id();
    if(initWatcher)
    {
        _stop = false;
        registerWatcher(initWatcher);
    }
};

EventLoop::~EventLoop()
{
    stop();
    for (auto &i : _watchers)
    {
        ::close(i.first);
        delete i.second;
    }
    for (auto &i : _idle_postmans)
    {
        delete i;
    }
    // _fdchanges是否还需要释放
}

Postman* EventLoop::newPostman(WATCHER_TYPE type, int fd)
{
    assert(fd>0);
    std::lock_guard lg(_mutex);
    if(_idle_postmans.empty())
    {
        Postman *p = new Postman(type);
        p->setFd(fd);
        p->setLoop(this);
        return p;
    }
    else
    {
        Postman *p = _idle_postmans.back();
        _idle_postmans.pop_back();
        p->setType(type);
        p->setFd(fd);
        p->setLoop(this);
        return p;
    }
}

void EventLoop::delPostman(Postman *p)
{
    if(p == nullptr) return;
    std::unique_lock ul(_mutex);
    if (_idle_postmans.size() < 64)
    {
        p->clear();
        _idle_postmans.push_back(p);
    }
    else
    {
        ul.unlock();
        // 析构函数中会调用clear
        delete p;
    }
}

// 每次loop之前调用
void EventLoop::fdReify()
{
    {
        std::lock_guard lg(_mutex);
        auto &reg_list = _fdchanges[0];

        for (IOWatcher *w : reg_list)
        {
            registerWatcher(w);
        }

        reg_list.clear();
    }
    auto &upd_list = _fdchanges[1];
    auto &unreg_list = _fdchanges[2];

    for (IOWatcher *w : upd_list)
    {
        updateWatcher(w);
    }
    for(IOWatcher *w: unreg_list)
    {
        unRegisterWatcher(w);
    }
    upd_list.clear();
    unreg_list.clear();
}

// 此函数只可能在两个线程中被调用，主线程和当前loop线程
int EventLoop::registerWatcher(IOWatcher *watcher)
{
    assert(!_stop);
    assert(watcher);

    int watcher_fd = watcher->getFd();
    assert(_watchers.find(watcher_fd) == _watchers.end());

    // new fd
    // 设置fd非阻塞
    int flags = ::fcntl(watcher_fd, F_GETFL, 0);
    ::fcntl(watcher_fd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC);
    std::cout << "[" << std::this_thread::get_id();
    std::cout << "][" << getThreadID();
    std::cout << "] add fd " << watcher_fd << " into loop" << std::endl;
    _watchers[watcher_fd] = watcher;
    watcher->setLoop(this);
    return _epollptr->addEvent(watcher_fd, watcher->getEvents(), watcher);
}

int EventLoop::updateWatcher(IOWatcher *watcher)
{
    assert(!_stop);
    assert(watcher);

    int fd = watcher->getFd();
    assert(_watchers.find(fd) != _watchers.end());

    assert(_watchers[fd] == watcher);
    // std::cout << "[+] mod event " << watcher->getEvents() << " in loop" << std::endl;
    return _epollptr->updateEvent(fd, watcher->getEvents(), watcher);
}

int EventLoop::unRegisterWatcher(IOWatcher *watcher)
{
    assert(!_stop);
    assert(watcher);

    int watcher_fd = watcher->getFd();
    assert(_watchers.find(watcher_fd) != _watchers.end());

    auto t = watcher->getType();
    if (t == LOCAL_POSTMAN || t == REMOTE_POSTMAN)
    {
        Postman *p = dynamic_cast<Postman *>(watcher);
        delPostman(p);
    }
    else
    {
        std::cout << "[!] not implemented " << std::endl;
    }
    

    std::cout << "[" << std::this_thread::get_id();
    std::cout << "][" << getThreadID();
    std::cout << "] del fd " << watcher_fd << " from loop" << std::endl;
    _watchers.erase(_watchers.find(watcher_fd));
    _epollptr->removeFd(watcher_fd);

    if(_watchers.size() == 0)
    {
        std::lock_guard lg(_mutex);
        _stop = true;
    }
    return E_OK;
}

void EventLoop::loop()
{
    if(!_firedEvents.capacity())
        return;

    do
    {
        std::unique_lock ul(_mutex);
        _cv.wait(ul, [this]{return !isStop();});
        ul.unlock();

        fdReify();

        int num = _epollptr->wait(&_firedEvents[0], _firedEvents.capacity(), 10);

        for (int i = 0; i < num; i++)
        {
            IOWatcher *w = static_cast<IOWatcher *>(_firedEvents[i].data.ptr);
            EVENT_TYPE events = _firedEvents[i].events;
            assert(_watchers.find(w->getFd()) != _watchers.end());

            // handle events
            if(w == nullptr)
            {
                std::cout << "unknown data_ptr" << std::endl;
            }
            else
            {
                w->handleEvent(events);
            }
        }
        timeoutCallback();
    } while (true);
}

void EventLoop::stop()
{
    std::lock_guard lg(_mutex);
    _stop = true;
}

void EventLoop::timeoutCallback()
{
    auto now = std::chrono::system_clock::now();
    for(auto& item : _watchers)
    {
        IOWatcher *w = item.second;
        if (w->getType() == LOCAL_POSTMAN || w->getType() == REMOTE_POSTMAN)
        {
            Postman *p = dynamic_cast<Postman *>(w);
            auto duration = now - p->getLastTime();
            // 超过1分钟未活动，就释放fd
            if (std::chrono::duration_cast<std::chrono::seconds>(duration).count() > 64)
            {
                p->handleClose();
            }
        }
    }
}

// addFd是唯一一个会被跨线程访问的函数，因此addFd函数访问的任何成员变量在任何地方都要加锁
// 因此_fdchanges、_idle_postman、_stop在其他地方也要加锁
Postman* EventLoop::addFd(WATCHER_TYPE t, int fd, sockaddr_in &addr, EVENT_TYPE init_event)
{
    assert(_watchers.find(fd) == _watchers.end());
    assert(t == LOCAL_POSTMAN || t == REMOTE_POSTMAN);
    Postman *p = newPostman(t, fd);
    assert(p);
    p->setStatus(CONNECTED);
    p->host = addr.sin_addr.s_addr;
    p->port = addr.sin_port;
    p->updateLastTime();
    p->setEvents(init_event);

    {
        std::lock_guard lg(_mutex);
        _fdchanges[0].push_back(p);
        _stop = false;
    }
    _cv.notify_one();
    return p;
}

int EventLoop::updateFd(int fd)
{
    assert(_watchers.find(fd) != _watchers.end());
    _fdchanges[1].push_back(_watchers[fd]);
    return E_OK;
}

// 由于deleteFd有可能同时被timeoutCallback和readall返回0主动handleClose调用
// 而且要“释放”postman实例，所以需要去重
int EventLoop::deleteFd(int fd)
{
    assert(_watchers.find(fd) != _watchers.end());
    auto &unreg_vec = _fdchanges[2];
    if (std::find(unreg_vec.begin(), unreg_vec.end(), _watchers[fd]) == unreg_vec.end())
    {
        unreg_vec.push_back(_watchers[fd]);
    }
    return E_OK;
}

void EventLoop::addIpCache(std::string &host, in_addr &addr)
{
    if(_ips_cache.size() >= 1024)
        _ips_cache.clear();
    
    _ips_cache[std::string(host)] = in_addr(addr);
}

in_addr EventLoop::getIpCache(std::string &host)
{
    if(_ips_cache.find(host) == _ips_cache.end())
        return in_addr{0};
    else
        return _ips_cache[host];
}