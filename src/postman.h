
#ifndef __POSTMAN_H__
#define __POSTMAN_H__

#include <queue>
#include <chrono>
#include "eventhandler.h"
#include "httpmsg.h"

enum POSTMAN_STATUS   // 此status是指upstream或者downstream的状态，不是指某个socket的连接状态
{
    CONNECTED = 2,   // 连接已建立。对于https，还未处理connect请求
    COMMUNICATING,   // 此Postman正在通信
    DISCONNECTED,    // 此Postman的连接已断开
    IDLE             // 此Postman的fd已关闭，需重新建立连接
};

enum TUNNEL_TYPE
{
    HTTP = 2,
    HTTPS,
    UNKNOWN
};

class Postman : public IOWatcher
{
public:
    explicit Postman(WATCHER_TYPE type);
    virtual ~Postman();

    virtual void handleEvent(EVENT_TYPE revents) override;

    // local是本地与Server的socket连接
    Postman *setPeer(std::string_view dst_host, int dst_port);
    int upstreamRead();    // 从local读取
    int downstreamWrite(); // 写往local

    // remote是Server与远程网站主机的连接
    Postman *peer_postman;
    int upstreamWrite();  // 写往remote
    int downstreamRead(); // 从remote读取
    int removeCurrentPeer();

    in_addr_t host;
    int       port;

    POSTMAN_STATUS status(){return _status;}
    void setStatus(POSTMAN_STATUS s){
        _status = s;
        if (peer_postman && _type == LOCAL_POSTMAN)
            peer_postman->setStatus(s);
    }
    TUNNEL_TYPE tunnelType(){return _tunnel_type;}
    void setTunnelType(TUNNEL_TYPE t){
        _tunnel_type = t;
        if (peer_postman && _type == LOCAL_POSTMAN)
            peer_postman->setTunnelType(t);
    }

    int sendMsgToPeer(std::string& msg);
    int enableReading();

    auto getLastTime() {return _last_time;}
    void updateLastTime(){_last_time = std::chrono::system_clock::now();};

    void clear();   // 清除内部状态
    
    virtual void handleClose() override; // 在读取写入时出错，删除self

private:

    std::string _inBuffer;         // 每次都将_inBuffer浅拷贝到_outBuffers，避免深拷贝
    std::queue<std::string> _outBuffers;

    POSTMAN_STATUS _status;
    TUNNEL_TYPE _tunnel_type;

    std::chrono::system_clock::time_point _last_time; // 此postman最后活动的时间点

    int __upstreamRead();

    int handleConnectMethod(HTTPMsgHeader&);
    int handleHTTPMsg(HTTPMsgHeader&);

    void appendOut(std::string& s);
};

#endif // __POSTMAN_H__