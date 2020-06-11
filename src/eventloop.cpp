#include <iostream>
#include <cassert>
#include <thread>
#include "postman.h"
#include "eventhandler.h"
#include "eventloop.h"
#include "utils.h"

EventLoop::EventLoop() : _epollptr(std::make_unique<Epoll>()),
                         _stop(false)
{
    _firedEvents.reserve(4);  // 预留空间，为epoll_wait准备
    _thread_id = std::this_thread::get_id();
};

EventLoop::~EventLoop()
{
    stop();
}

// 此函数只可能在两个线程中被调用，主线程和当前loop线程
int EventLoop::registerWatcher(IOWatcher *watcher)
{
    assert(!_stop);
    if (watcher == nullptr)
        return E_CANCEL;

    {
        std::lock_guard lg(_mutex);
        int watcher_fd = watcher->getFd();

        if (std::find(_fds.begin(), _fds.end(), watcher_fd) == _fds.end())
        {
            // new fd
            std::cout << "[" << std::this_thread::get_id();
            std::cout << "][" << getThreadID();
            std::cout << "] add fd " << watcher_fd << " into loop" << std::endl;
            _fds.push_back(watcher_fd);
            _epollptr->addEvent(watcher_fd, watcher->getEvents(), watcher);
        }
        else
            //TODO: updateWatcher
            return E_CANCEL;
    }
    if(_fds.size() == 1)
        _cv.notify_one();
    return E_OK;
}

int EventLoop::updateWatcher(IOWatcher *watcher)
{
    assert(!_stop);
    if(watcher == nullptr)
        return E_CANCEL;

    {
        std::lock_guard lg(_mutex);
        if (std::find(_fds.begin(), _fds.end(), watcher->getFd()) == _fds.end())
            // TODO: addWatcher
            return E_CANCEL;
        else
        {
            // std::cout << "[+] mod event " << watcher->getEvents() << " in loop" << std::endl;
            return _epollptr->updateEvent(watcher->getFd(), watcher->getEvents(), watcher);
        }
    }
}

int EventLoop::unRegisterWatcher(IOWatcher *watcher)
{
    assert(!_stop);
    if(watcher == nullptr)
        return E_CANCEL;

    {
        std::lock_guard lg(_mutex);
        int watcher_fd = watcher->getFd();
        auto fd_index = std::find(_fds.begin(), _fds.end(), watcher_fd);
        if (fd_index == _fds.end())
            return E_CANCEL;

        _fds.erase(fd_index);
        std::cout << "[" << std::this_thread::get_id();
        std::cout << "][" << getThreadID();
        std::cout << "] del fd " << watcher_fd << " from loop" << std::endl;
        return _epollptr->removeFd(watcher_fd);
    }
}

// loop函数不用加锁
void EventLoop::loop()
{
    if(!_firedEvents.capacity())
        _stop = true;

    do
    {
        std::unique_lock ul(_mutex);
        _cv.wait(ul, [&]{return _fds.size()>0;});
        ul.unlock();
        // while(_fds.size() == 0)
        //     std::this_thread::sleep_for(std::chrono::seconds(1));
        int num = _epollptr->wait(&_firedEvents[0], _firedEvents.capacity(), 10);
        for (int i = 0; i < num; i++)
        {
            IOWatcher *w = static_cast<IOWatcher *>(_firedEvents[i].data.ptr);
            EVENT_TYPE events = _firedEvents[i].events;

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
    } while (!_stop);
}

void EventLoop::stop()
{
    _stop = true;
}

void EventLoop::timeoutCallback()
{
    Server *s = Server::getInstance();
    auto now = std::chrono::system_clock::now();
    for(auto& fd : _fds)
    {
        auto w_iter = s->watchers.find(fd);
        if(w_iter == s->watchers.end()) continue;

        IOWatcher *w = w_iter->second;
        if (w->getType() == LOCAL_POSTMAN || w->getType() == REMOTE_POSTMAN)
        {
            Postman *p = dynamic_cast<Postman *>(w);
            auto duration = now - p->getLastTime();
            // 超过1分钟未活动，就释放fd
            if (std::chrono::duration_cast<std::chrono::seconds>(duration).count() > 64)
            {
                s->delWatcher(p);
            }
        }
    }
}