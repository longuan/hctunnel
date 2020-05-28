
#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <vector>
#include <memory>
#include "epoller.h"


class EventLoop
{
private:
    std::unique_ptr<Epoll> _epollptr;
    bool _stop; // 每个EventLoop只属于一个线程，因此不需要设为atomic
    std::vector<int>             _fds;   // 此loop管理的所有fd
    std::vector<epoll_event> _firedEvents;   // 存放已触发的事件

public:
    EventLoop();
    ~EventLoop() =default;
    void loop(int flag);
    void stop();

    // 新增client fd
    int registerWatcher(IOWatcher *watcher);
    // 新增event或者删除event
    int updateWatcher(IOWatcher *watcher);
    // 解除client fd
    int unRegisterWatcher(IOWatcher *watcher);
};

#endif // __EVENTLOOP_H__