#ifndef __SERVER_H__
#define __SERVER_H__

#include <mutex>
#include <unordered_map>
#include <chrono>
#include "utils.h"

class EventLoop;
class Acceptor;
class IOWatcher;
class Postman;
class ThreadPool;

// Server只有一个实例，会在多个线程中同时被调用
class Server
{
private:
    EventLoop                          *_main_loop;
    ThreadPool                         *_io_loops;
    Acceptor                           *_acceptor;
    std::vector<Postman*>               _idle_postmans;
    std::mutex                          _mutex;
    std::pair<in_addr_t, int>           _last_count; // 用来防止同一ip瞬间发起的大量连接，导致fd用尽

    Server();
    ~Server();

    static Server *_global_server;
    void delPostman(Postman *p);

public:
    std::unordered_map<int, IOWatcher *> watchers; // 或者使用tuple

    static Server *getInstance(){return _global_server;};

    Server(const Server &) =delete;
    Server& operator=(const Server&) =delete;

    int start(int port = 8888);
    // void stop();

    void newConnectionCallback(WATCHER_TYPE, int, sockaddr_in&);

    // 这四个方法以及delPostman需要保证线程安全
    // _idle_postmans、watchers需要被加锁保护
    Postman *newPostman(WATCHER_TYPE type, int fd, EventLoop *loop);
    int addWatcher(IOWatcher *w); // 新fd，加入loop
    int updWatcher(IOWatcher *w); // 更新events或者更新cb
    int delWatcher(IOWatcher *w); // 删除fd
};

#endif // __SERVER_H__