
#ifndef __EVENTHANDLER_H__
#define __EVENTHANDLER_H__

#include <unordered_map>
#include <functional>
#include "server.h"

class EventLoop;

class IOWatcher
{
public:
    virtual void handleEvent(int revents) = 0; // handleEvent根据event调用不同的函数，实现无callback化

    explicit IOWatcher(WATCHER_TYPE t):_fd(0),_events(0),_loop(nullptr),_type(t){};
    virtual ~IOWatcher() =default;
    IOWatcher(const IOWatcher& ) =delete;
    IOWatcher& operator=(const IOWatcher&) =delete;

    int setFd(int fd){if(!_fd)_fd=fd;return _fd;}
    int getFd() const {return _fd;};
    EventLoop* setLoop(EventLoop *loop){if(_loop==nullptr)_loop=loop;return _loop;}
    EventLoop* getLoop() const {return _loop;}
    int addEvent(int event)
    {
        if(event & _events)
            return E_CANCEL;
        _events |= event;
        Server *s = Server::getInstance();
        return s->updWatcher(this);
    }
    int delEvent(int event)
    {
        if(event & _events)
        {
            _events &= (~event);
            Server *s = Server::getInstance();
            return s->updWatcher(this);
        }
        return E_CANCEL;
    }
    int getEvents() const {return _events;}
    WATCHER_TYPE getType() const { return _type; }
    void setType(WATCHER_TYPE t) { _type = t; }

protected:
    int _fd; // fd由server负责close
    int _events; // 存储关于这个fd的所有感兴趣的event，虽然也可以通过遍历callbacks的key得到
    EventLoop *_loop;
    WATCHER_TYPE _type;
};
#endif