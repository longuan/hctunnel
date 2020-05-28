#include "httpmsg.h"
#include "utils.h"

int HTTPMsgHeader::parseHeader(std::string_view msg)
{
    // 清除已有的状态
    {
        _msg_type = HTTPS_DATA;
        _dst_port = 0;
        _content_length_value = 0;
        _msg = msg;
    }
    auto msg_header_end = msg.find("\r\n\r\n");
    if(msg_header_end == std::string_view::npos)
        return E_CANCEL;
    auto msg_header = msg.substr(0, msg_header_end);
    auto msg_reqline_end = msg_header.find("\n");
    if(msg_reqline_end == std::string_view::npos)
        return E_CANCEL;

    std::string_view reqline = msg_header.substr(0, msg_reqline_end);
    auto space1_idx = reqline.find(' ');
    if(space1_idx == std::string_view::npos)
        return E_CANCEL;
    auto method = reqline.substr(0, space1_idx);
    if (method == "GET")
        _msg_type = HTTP_GET;
    else if (method == "POST")
        _msg_type = HTTP_POST;
    else if(method == "CONNECT")
        _msg_type = HTTP_CONNECT;
    else
    {
        _msg_type = HTTPS_DATA;
        return E_OK;
    }

    _dst_host = headerValue(msg_header, "Host: ");
    if (_dst_host.size() == 0)
    {
        // 否则从request line中的url中解析出host:port
        auto space2_idx = reqline.find(' ', space1_idx+1);
        auto url = reqline.substr(space1_idx+1, space2_idx-space1_idx-1);
        return E_ERROR;
    }
    auto i = _dst_host.find(':');
    if (i != decltype(_dst_host)::npos)
    {
        _dst_port = std::stoi(std::string(_dst_host.substr(i + 1, _dst_host.size() - i - 1)));
        _dst_host = _dst_host.substr(0, i);
    }
    else
    {
        _dst_port = 80;
    }
    
    if (_msg_type == HTTP_POST)
    {
        std::string_view content_length = headerValue(msg_header, "Content-Length: ");
        if(content_length.size() == 0)
        {
            // "[!] POST without content-length"
            return E_ERROR;
        }
        else
        {
            _content_length_value = std::stoi(std::string(content_length));
        }
    }
    return E_OK;
}