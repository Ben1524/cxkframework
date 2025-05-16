#include "http_session.h"
#include "http_parser.h"

namespace cxk{
namespace http{

static Logger::ptr g_logger = CXK_LOG_NAME("system");

HttpSession::HttpSession(Socket::ptr sock, bool owner) : SocketStream(sock, owner){

}

HttpRequest::ptr HttpSession::recvRequest(){

    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    CXK_LOG_DEBUG(g_logger) << "HttpSession::recvRequest() call" << " buff_size:" << buff_size;

    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr){
        delete[] ptr;
    });

    char* data = buffer.get();
    int offset = 0;

    // 循环读取数据
    do{
        int len = read(data + offset, buff_size - offset);
        CXK_LOG_DEBUG(g_logger) << "HttpSession::recvRequest() read len:" << len;
        if(len <= 0){
            close();
            return nullptr;
        }

        len += offset;
        size_t nparse = parser->execute(data, len);
        if(parser->hasError()){
            CXK_LOG_ERROR(g_logger) << "parse http request error";
            close();
            return nullptr;
        }

        offset = len - nparse;
        if(offset == (int)buff_size){
            close();
            return nullptr;
        }

        if(parser->isFinished()){
            break;
        } 
    } while(true);


    // 处理请求体
    uint64_t length = parser->getContentLength();
    if(length > 0){
        std::string body;
        body.resize(length);

        int len = 0;
        // 将已读取的数据复制到body中
        if(length >= (uint64_t)offset){
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        // 读取剩余的数据
        length -= offset;
        if(length > 0){
            if(readFixSize(&body[len], length) <= 0){
                close();
                return nullptr;
            }
        }
        // 设置请求体
        parser->getData()->setBody(body);
    }

    // 处理连接状态
    std::string keep_alive = parser->getData()->getHeader("Connection");
    if(strcasecmp(keep_alive.c_str(), "keep-alive") == 0){
        parser->getData()->setClose(false);
    }
    return parser->getData();
}
    
    
int HttpSession::sendResponse(HttpResponse::ptr rsp){
     std::stringstream ss;
     ss << *rsp;
     std::string data = ss.str();
     return writeFixSize(data.c_str(), data.size());
}

}


}