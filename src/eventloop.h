
#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <thread>
#include "epoller.h"

class IOWatcher;

class EventLoop
{
private:
    std::unique_ptr<Epoll>      _epollptr;
    bool                        _stop; // 每个EventLoop只属于一个线程，因此不需要设为atomic
    std::vector<int>            _fds;   // 此loop管理的所有fd
    std::vector<epoll_event>    _firedEvents;   // 存放已触发的事件
    std::mutex                  _mutex;
    std::condition_variable     _cv;
    std::thread::id             _thread_id;
    std::unordered_map<std::string, in_addr> _ips_cache; // 缓存域名与ip的对应关系

public:
    EventLoop();
    ~EventLoop();
    void loop();
    void stop();

    void timeoutCallback();

    // 新增client fd
    int registerWatcher(IOWatcher *watcher);
    // 新增event或者删除event
    int updateWatcher(IOWatcher *watcher);
    // 解除client fd
    int unRegisterWatcher(IOWatcher *watcher);

    std::thread::id getThreadID(){return _thread_id;}

    void addIpCache(std::string& host, in_addr& addr);
    in_addr getIpCache(std::string& host);
};

#endif // __EVENTLOOP_H__