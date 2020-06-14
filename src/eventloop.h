
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
class Postman;

class EventLoop
{
private:
    std::unique_ptr<Epoll>                   _epollptr;
    bool                                     _stop;
    std::unordered_map<int, IOWatcher *>     _watchers;  // 所有需要管理的fd
    std::vector<IOWatcher*>                  _fdchanges[3]; // 0:reg. 1:upd. 2:unreg.
    std::vector<epoll_event>                 _firedEvents;  // 存放已触发的事件
    std::mutex                               _mutex;
    std::condition_variable                  _cv;
    std::thread::id                          _thread_id;
    std::unordered_map<std::string, in_addr> _ips_cache; // 缓存域名与ip的对应关系
    std::vector<Postman *>                   _idle_postmans; // 缓存postman实例

    void delPostman(Postman *p);

    void fdReify();

    // 新增client fd
    int registerWatcher(IOWatcher *watcher);
    // 新增event或者删除event
    int updateWatcher(IOWatcher *watcher);
    // 解除client fd
    int unRegisterWatcher(IOWatcher *watcher);

public:
    explicit EventLoop(IOWatcher *initWatcher=nullptr);
    ~EventLoop();
    void loop();
    void stop();

    Postman *newPostman(WATCHER_TYPE type, int fd);

    void timeoutCallback();

    Postman *addFd(WATCHER_TYPE t, int fd, sockaddr_in &addr, EVENT_TYPE init_event);
    int updateFd(int fd);
    int deleteFd(int fd);

    std::thread::id getThreadID(){return _thread_id;}

    void addIpCache(std::string& host, in_addr& addr);
    in_addr getIpCache(std::string& host);

    bool isStop(){return _stop;}
};

#endif // __EVENTLOOP_H__