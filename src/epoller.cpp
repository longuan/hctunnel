#include <unistd.h> // close()
#include <cerrno>
#include <fcntl.h>
#include "epoller.h"

int Epoll::addEvent(int fd, EVENT_TYPE event, void *data_ptr)
{
    epoll_event e = {};
    e.data.ptr = data_ptr;
    e.events = event;
    int result = ::epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &e);
    if(result == -1)
        // TODO: 根据errno细分 EEXIST、ENOENT、EBADF等等
        return E_ERROR;
    return E_OK;
}

int Epoll::updateEvent(int fd, EVENT_TYPE event, void *data_ptr)
{
    epoll_event e = {};
    e.data.ptr = data_ptr;
    e.events = event;
    int result = ::epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &e);
    if (result == -1)
        // TODO: 根据errno细分 EEXIST、ENOENT、EBADF等等
        return E_ERROR;
    return E_OK;
}

int Epoll::removeFd(int fd)
{
    int result = ::epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, NULL);
    if(result == -1)
        // TODO: 根据errno细分 EEXIST、ENOENT、EBADF等等
        return E_ERROR;
    return E_OK;
}


// copy from libev/ev_epoll.c
Epoll::Epoll() : _epollfd(::epoll_create1(EPOLL_CLOEXEC))
{
    if (_epollfd < 0 && (errno == EINVAL || errno == ENOSYS))
    {
        _epollfd = epoll_create(256);
        if(_epollfd >= 0)
            fcntl(_epollfd, F_SETFD, FD_CLOEXEC);
        else
            FATAL_ERROR("Epoll::Epoll()");
    }
}

Epoll::~Epoll()
{
    ::close(_epollfd);
}
