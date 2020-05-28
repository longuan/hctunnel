
#ifndef __HTTPMSG_H__
#define __HTTPMSG_H__

#include <string>

enum MSGMethod
{
    HTTP_GET = 10,
    HTTP_POST,
    HTTP_CONNECT,
    HTTP_RESPONSE,
    HTTPS_DATA
};

class HTTPMsgHeader
{
private:
    MSGMethod _msg_type;
    std::string_view _msg;
    std::string_view _dst_host;
    int              _dst_port;
    std::string::size_type _content_length_value;
public:
    HTTPMsgHeader():_msg_type(HTTPS_DATA),_msg(std::string_view(0,0)),
                    _dst_host(std::string_view(0,0)), _dst_port(0), _content_length_value(0){};
    MSGMethod getType()const {return _msg_type;}
    std::string_view getMsg() {return _msg;}
    std::string_view getHost() {return _dst_host;}
    std::string::size_type getContentLength()const {return _content_length_value;}
    int getPort()const {return _dst_port;}
    int parseHeader(std::string_view msg_header);
};


#endif // __HTTPMSG_H__