#ifndef __SERVER_H__
#define __SERVER_H__

#include <mutex>
#include <unordered_map>
#include "utils.h"

class EventLoop;
class Acceptor;
class IOWatcher;
class Postman;

class Server
{
private:
    std::unordered_map<int, IOWatcher*> _watchers;  // 或者使用tuple
    EventLoop *                _loop; // TODO: 实现多个loop
    Acceptor *              _acceptor;
    int                      _status;
    std::vector<Postman*> _idle_postmans;

    Server();
    ~Server();

    static Server *global_server;
    void delPostman(Postman *p);

public:
    static Server *getInstance();

    Server(const Server &) =delete;
    Server& operator=(const Server&) =delete;

    int run(int port = 8888);
    void stop();

    void newConnectionCallback(WATCHER_TYPE, int, sockaddr_in&);

    Postman *newPostman(WATCHER_TYPE type, int fd, EventLoop *loop);
    int addWatcher(IOWatcher *w); // 新fd，加入loop
    int updWatcher(IOWatcher *w); // 更新events或者更新cb
    int delWatcher(IOWatcher *w); // 删除fd
};

#endif // __SERVER_H__