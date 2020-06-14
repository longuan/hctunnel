
#include "eventhandler.h"
#include "eventloop.h"

int IOWatcher::addEvent(EVENT_TYPE event)
{
    if (event & _events)
        return E_CANCEL;
    _events |= event;
    return _loop->updateFd(_fd);
}

int IOWatcher::delEvent(EVENT_TYPE event)
{
    if (event & _events)
    {
        _events &= (~event);
        return _loop->updateFd(_fd);
    }
    return E_CANCEL;
}