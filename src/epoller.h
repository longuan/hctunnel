
#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <sys/epoll.h>
#include "utils.h"

class Epoll
{
private:
    int _epollfd;
public:
    Epoll();
    // watcher 作为epoll_event.data.ptr
    int addEvent(int fd, EVENT_TYPE event, void *data_ptr);
    int updateEvent(int fd, EVENT_TYPE event, void *data_ptr);
    int removeFd(int fd);
    int wait(epoll_event *events, int maxevent, int timeout)
    {
        return epoll_wait(_epollfd, events, maxevent, timeout);
    };
    ~Epoll();

};


#endif // __EPOLLER_H__