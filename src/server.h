#ifndef __SERVER_H__
#define __SERVER_H__

#include "utils.h"

class EventLoop;
class Acceptor;
class IOWatcher;
class Postman;
class ThreadPool;

class Server
{
private:
    EventLoop                          *_main_loop;
    ThreadPool                         *_io_loops;
    Acceptor                           *_acceptor;
    std::pair<in_addr_t, int>           _last_count; // 用来防止同一ip瞬间发起的大量连接，导致fd用尽

    Server();
    ~Server();

    static Server *_global_server;

public:

    static Server *getInstance(){return _global_server;};

    Server(const Server &) =delete;
    Server& operator=(const Server&) =delete;

    int start(int port = 8888);
    // void stop();

    void newConnectionCallback(WATCHER_TYPE, int, sockaddr_in&);
};

#endif // __SERVER_H__