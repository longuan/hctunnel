#include <sys/epoll.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <cassert>
#include "server.h"
#include "postman.h"

using std::cout, std::endl, std::vector;

Postman::Postman(WATCHER_TYPE type) : IOWatcher(type), peer_postman(nullptr),
                                    _status(IDLE), _tunnel_type(UNKNOWN)
{
    host = 0;
    port = 0;
}

Postman::~Postman()
{
    if(_type == LOCAL_POSTMAN)
    {
        removeCurrentPeer();
    }
}

int Postman::removeCurrentPeer()
{
    assert(_type = LOCAL_POSTMAN);
    if(peer_postman)
    {
        Server::getInstance()->delWatcher(peer_postman);
    }
    setStatus(DISCONNECTED);
    setTunnelType(UNKNOWN);
    peer_postman = nullptr;
    return E_OK;
}

int Postman::sendMsgToPeer(std::string_view msg)
{
    if(peer_postman == nullptr)
        return E_ERROR;
    peer_postman->appendOut(msg);
    return peer_postman->addEvent(EPOLLOUT);
}

void Postman::appendOut(std::string_view sv)
{
    _outBuffers.push(std::string(sv));
}

int Postman::enableReading()
{
    return addEvent(EPOLLIN);
}

void Postman::handleEvent(int revents)
{
    if((revents & _events) == 0)
        return;

    if(revents & EPOLLIN)
    {
        if(_type == LOCAL_POSTMAN)
            upstreamRead();
        else
            downstreamRead();
    }
    if (revents & EPOLLOUT)
    {
        if(_type == REMOTE_POSTMAN)
            upstreamWrite();
        else
            downstreamWrite();
    }
}

// upstreamRead的数据包有三种情况：
// 1. 接收到CONNECT请求：需要建立新的TUNNEL通道，且将此tunnel设置为https通道
// 2. 接收到https数据：之前必须已经接收到CONNECT请求
// 3. 接收到HTTP请求：需要判断是否需要建立新的TUNNEL通道
// 此函数处理1和3两种
// 一个http message 由四部分组成：request line、headers、an empty line、Optional HTTP message body data
int Postman::__upstreamRead()
{
    HTTPMsgHeader msg_header;

    if(msg_header.parseHeader(_inBuffer) == E_ERROR)
    {
        std::cout << "[!] cannot parse host:port from HostStr" << std::endl;
        std::cout << msg_header.getMsg() << std::endl;
        return E_ERROR;
    }
    if (msg_header.getType() == HTTPS_DATA)
    {   
        if (status() == COMMUNICATING )
        {
            sendMsgToPeer(std::string_view(_inBuffer));
            return E_OK;
        }
        else
        {
            std::cout << "HTTPS tunnel not connect" << std::endl;
            return E_ERROR;
        }
    }
    else if (msg_header.getType() == HTTP_CONNECT)
    {
        // 删除现有peer_postman
        removeCurrentPeer();
        handleConnectMethod(msg_header);
    }
    else // 否则此消息就是HTTP请求
    {
        handleHTTPMsg(msg_header);
    }
    return E_OK;
}

int Postman::handleConnectMethod(HTTPMsgHeader& msg)
{
    assert(msg.getType() == HTTP_CONNECT);
    auto peer = setPeer(msg.getHost(), msg.getPort());
    if(peer == nullptr)
    {
        std::cout << "[!] unable to connect target domain" << std::endl;
        setStatus(DISCONNECTED);
        setTunnelType(UNKNOWN);
        return E_CANCEL;
    }
    else
    {
        setStatus(CONNECTED);
        setTunnelType(HTTPS);
        std::cout << "[+] connect to target domain successfully" << std::endl;
        std::string response{"HTTP/1.1 200 Connection Established\r\n"
                            "FiddlerGateway : Direct\r\n"
                            "StartTime : 03:29:24.878\r\n"
                            "Connection : close\r\n\r\n"};
        appendOut(std::string_view(response));
        addEvent(EPOLLOUT);
        setStatus(COMMUNICATING);
        return E_OK;
    }
}

int Postman::handleHTTPMsg(HTTPMsgHeader& msg)
{
    assert(_type == LOCAL_POSTMAN);
    auto sv = msg.getHost();
    if (peer_postman && (inet_addr(sv.data()) == peer_postman->host) &&
        (msg.getPort() == peer_postman->port))
    {
        setTunnelType(HTTP);
        return sendMsgToPeer(msg.getMsg());
    }
    else
    {
        removeCurrentPeer();
        auto peer = setPeer(msg.getHost(), msg.getPort());
        if (peer == nullptr)
        {
            std::cout << "[!] unable to connect target domain 2" << std::endl;
            setStatus(DISCONNECTED);
            setTunnelType(UNKNOWN);
            return E_CANCEL;
        }
        else
        {
            setStatus(COMMUNICATING);
            return sendMsgToPeer(msg.getMsg());
        }
    }
}

int Postman::upstreamRead()
{
    auto nread = readall(_fd, _inBuffer);
    if(nread == 0)
    {
        // 对端已经关闭或者读取出错，统一按照对端已经关闭处理
        Server *s = Server::getInstance();
        return s->delWatcher(this);
    }
    else if(nread == -1)
    {
        std::cout << "[!] read error" << std::endl;
        return E_ERROR;
    }
    else
    {
        std::cout << "[@] recv message from local, length: " << nread << std::endl;
        __upstreamRead();
        _inBuffer.clear();
        return E_OK;
    }
}

int Postman::upstreamWrite()
{
    while (!_outBuffers.empty())
    {
        auto s = _outBuffers.front();
        _outBuffers.pop();
        size_t nwrite = writeall(_fd, std::string_view(s));
        if(nwrite <= 0)
        {
            std::cout << "[!] write error" << std::endl;
            peer_postman->peer_postman = nullptr;
            Server *s = Server::getInstance();
            return s->delWatcher(this);
        }
        std::cout << "[@] send message to remote length: " << nwrite << std::endl;
    }
    delEvent(EPOLLOUT);
    return enableReading();
}

int Postman::downstreamRead()
{
    auto nread = readall(_fd, _inBuffer);
    if (nread == 0)
    {
        // 对端已经关闭或者读取出错，统一按照对端已经关闭处理
        peer_postman->peer_postman = nullptr;
        Server *s = Server::getInstance();
        return s->delWatcher(this);
    }
    else if (nread == -1)
    {
        std::cout << "[!] read error" << std::endl;
        return E_ERROR;
    }
    else
    {
        std::cout << "[@] recv message from remote, length: " << nread << std::endl;
        assert(_type == REMOTE_POSTMAN);
        sendMsgToPeer(std::string_view(_inBuffer));
        _inBuffer.clear();
        return E_OK;
    }
}

int Postman::downstreamWrite()
{  // 跟upstreamWrite一模一样
    while (!_outBuffers.empty())
    {
        auto s = _outBuffers.front();
        _outBuffers.pop();
        size_t nwrite = writeall(_fd, std::string_view(s));
        if (nwrite <= 0)
        {
            std::cout << "[!] write error" << std::endl;
            Server *s = Server::getInstance();
            return s->delWatcher(this);
        }
        std::cout << "[@] send message to local, length: " << nwrite << std::endl;
    }
    delEvent(EPOLLOUT);
    return enableReading();
}

Postman *Postman::setPeer(std::string_view dst_host, int dst_port)
{
    std::vector<in_addr> ips;
    std::string host(dst_host);
    getHostIp(host.c_str(), ips);

    int peer_fd = 0;
    in_addr_t addr;
    for (auto &i : ips)
    { // 尝试哪个ip可用
        if((peer_fd = connectHost(i, dst_port)) > 0)
        {
            addr = i.s_addr;
            break;
        }
    }
    if(peer_fd <= 0)
        return nullptr;

    // 从Server取得Postman
    Server *s = Server::getInstance();
    Postman *p = s->newPostman(REMOTE_POSTMAN, peer_fd, _loop);
    p->host = addr;
    p->port = dst_port;
    p->peer_postman = this;
    peer_postman = p;
    return p;
}

void Postman::clear()
{
    _status = IDLE;
    _tunnel_type = UNKNOWN;
    std::queue<std::string>().swap(_outBuffers);
    _inBuffer.clear();
    host = 0;
    port = 0;
    peer_postman = nullptr;
    _events = 0;
}