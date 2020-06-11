#ifndef __ACCEPTOT_H__
#define __ACCEPTOR_H__

#include "eventhandler.h"


class Acceptor : public IOWatcher
{
private:
    int _listenport;
public:
    Acceptor():IOWatcher(ACCEPTOR),_listenport(0){};
    int start(int port);
    void stop();
    virtual ~Acceptor() =default;

    virtual void handleEvent(EVENT_TYPE revents) override;
    virtual void handleClose() override;

    int acceptClient();

};


#endif // __ACCEPTOR_H__