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
    std::unordered_map<int, IOWatcher*> _watchers;  // 或者使用tuple
    EventLoop                          *_main_loop;
    ThreadPool                         *_io_loops;
    Acceptor                           *_acceptor;
    std::vector<Postman*>               _idle_postmans;
    std::mutex                          _mutex;

    Server();
    ~Server();

    static Server *_global_server;
    void delPostman(Postman *p);

public:
    static Server *getInstance(){return _global_server;};

    Server(const Server &) =delete;
    Server& operator=(const Server&) =delete;

    int start(int port = 8888);
    // void stop();

    void newConnectionCallback(WATCHER_TYPE, int, sockaddr_in&);
    void timeoutCallback(int fd, std::chrono::system_clock::time_point& now);

    Postman *newPostman(WATCHER_TYPE type, int fd, EventLoop *loop);
    int addWatcher(IOWatcher *w); // 新fd，加入loop
    int updWatcher(IOWatcher *w); // 更新events或者更新cb
    int delWatcher(IOWatcher *w); // 删除fd
};

#endif // __SERVER_H__