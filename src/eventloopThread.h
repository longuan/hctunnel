#ifndef __EVENTLOOPTHREAD_H__
#define __EVENTLOOPTHREAD_H__

#include <thread>
#include "eventloop.h"

class EventLoopThread
{
private:
    std::thread         _thread;
    EventLoop *         _loop;

public:
    EventLoopThread() : _thread(std::thread([this]{this->threadFunc();})){};
    ~EventLoopThread(){
        _loop->stop();
        _thread.join();
        delete _loop;
    };

    void threadFunc(){
        _loop = new EventLoop();
        _loop->loop();
    };

    EventLoop* getLoop(){return _loop;}
};

#endif