#include "eventloopThread.h"
#include "threadpool.h"

ThreadPool::ThreadPool(int num):_numThreads(num),_next(0)
{
    // _numThreads = std::thread::hardware_concurrency();
    _threads.reserve(_numThreads);
    for (int i = 0; i < _numThreads; i++)
    {
        _threads.push_back(new EventLoopThread());
    }
}

ThreadPool::~ThreadPool()
{
    for(auto t:_threads)
    {
        delete t;
    }
}

EventLoop *ThreadPool::getNextLoop()
{
    ++_next;
    if(_next == _numThreads)
        _next = 0;
    
    return _threads[_next]->getLoop();
}