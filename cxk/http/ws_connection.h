#pragma once

#include "http_connection.h"
#include "ws_session.h"


namespace cxk{
namespace http{


class WSConnection : public HttpConnection{
public:
    using ptr = std::shared_ptr<WSConnection>;
    WSConnection(Socket::ptr sock, bool owner = true);


    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(const std::string& url, 
        uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {});

    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(Uri::ptr uri, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {});

    WSFrameMessage::ptr recvMessage();

    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);
    int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);

    int32_t ping();
    int32_t pong();
};





}
}