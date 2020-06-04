
#include "server.h"


int main(int argc, char *argv[])
{
    Server *s = Server::getInstance();
    s->start();
}