#ifndef __ACCEPTOT_H__
#define __ACCEPTOR_H__

#include "eventhandler.h"
#include <fcntl.h>

class Acceptor : public IOWatcher
{
private:
    int _listenport;
    int _idlefd;
public:
    Acceptor() : IOWatcher(ACCEPTOR), _listenport(0), _idlefd(::open("/dev/null", O_RDONLY | O_CLOEXEC))
    {
        _fd = ::socket(PF_INET, SOCK_STREAM, 0);
        if (_fd < 0)
            FATAL_ERROR("Acceptor socket error");
    };
    int start(int port);
    void stop();
    virtual ~Acceptor() =default;

    virtual void handleEvent(EVENT_TYPE revents) override;
    virtual void handleClose() override;

    int acceptClient();

};


#endif // __ACCEPTOR_H__