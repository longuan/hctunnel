
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <vector>
#include <thread>

class EventLoopThread;

class ThreadPool
{
private:
    int _numThreads;
    int _next;
    std::vector<EventLoopThread *> _threads;

public:
    explicit ThreadPool(int num = 4);
    ~ThreadPool();

    EventLoop *getNextLoop();
};


#endif // __THREADPOOL_H__