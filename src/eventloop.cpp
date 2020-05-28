#include <iostream>
#include "eventhandler.h"
#include "eventloop.h"
#include "utils.h"

EventLoop::EventLoop() : _epollptr(std::make_unique<Epoll>()),
                         _stop(false)
{
    _firedEvents.reserve(4);  // 预留空间，为epoll_wait准备
};

int EventLoop::registerWatcher(IOWatcher *watcher)
{
    if (watcher == nullptr)
        return E_CANCEL;

    int watcher_fd = watcher->getFd();
    if (std::find(_fds.begin(), _fds.end(), watcher_fd) == _fds.end())
    {
        // new fd
        std::cout << "[+] add fd " << watcher_fd << " into loop" << std::endl;
        _fds.push_back(watcher_fd);
        return _epollptr->addEvent(watcher_fd, watcher->getEvents(), watcher);
    }
    else
        //TODO: updateWatcher
        return E_CANCEL;
}

int EventLoop::updateWatcher(IOWatcher *watcher)
{
    if(watcher == nullptr)
        return E_CANCEL;

    if(std::find(_fds.begin(), _fds.end(), watcher->getFd()) == _fds.end())
        // TODO: addWatcher
        return E_CANCEL;
    else
    {
        // std::cout << "[+] mod event " << watcher->getEvents() << " in loop" << std::endl;
        return _epollptr->updateEvent(watcher->getFd(), watcher->getEvents(), watcher);
    }
}

int EventLoop::unRegisterWatcher(IOWatcher *watcher)
{
    if(watcher == nullptr)
        return E_CANCEL;

    int watcher_fd = watcher->getFd();
    auto fd_index = std::find(_fds.begin(), _fds.end(), watcher_fd);
    if (fd_index == _fds.end())
        return E_CANCEL;

    _fds.erase(fd_index);
    std::cout << "[+] del fd " << watcher_fd << " from loop" << std::endl;
    return _epollptr->removeFd(watcher_fd);
}

// flag为1，代表wait一次就返回，flag为0，代表无限wait
void EventLoop::loop(int flag)
{
    if(!_firedEvents.capacity())
        _stop = true;

    do
    {
        int num = _epollptr->wait(&_firedEvents[0], _firedEvents.capacity(), -1);
        // std::cout << "[*] got " << num << " events" << std::endl;
        for (int i = 0; i < num; i++)
        {
            IOWatcher *w = static_cast<IOWatcher *>(_firedEvents[i].data.ptr);
            int events = _firedEvents[i].events;
            // std::cout << events << " from " << w->getFd() << std::endl;

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
    } while (!(_stop||flag));
}

void EventLoop::stop()
{
    _stop = true;
}