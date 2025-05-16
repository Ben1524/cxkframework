#pragma once

#include "cxk/stream/socket_stream.h"
#include "http.h"
#include "http_parser.h"

namespace cxk{
namespace http{


/// @brief  HTTP Session封装
class HttpSession : public SocketStream{
public:
    using ptr = std::shared_ptr<HttpSession>;

    /// @brief          构造函数
    /// @param sock     Socket类型
    /// @param owner    是否托管
    HttpSession(Socket::ptr sock, bool owner = true);

    /// @brief          接收HTTP请求
    HttpRequest::ptr recvRequest();

    /// @brief          发送HTTP响应
    /// @param rsp      HTTP响应
    /// @return         -1: 发送失败, 0: 对方关闭连接, >0: 发送成功
    int sendResponse(HttpResponse::ptr rsp);

};


}





}