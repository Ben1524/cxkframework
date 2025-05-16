#pragma once
#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace cxk{
namespace http{


/// @brief  HTTP请求解析器
class HttpRequestParser{
public:
    using ptr = std::shared_ptr<HttpRequestParser>;

    /// @brief  构造函数
    HttpRequestParser();

    /// @brief      解析协议
    /// @param data 协议文本内存
    /// @param len  协议文本长度
    /// @return     返回实际解析的长度，并且将已解析的数据移除
    size_t execute(char* data, size_t len);

    /// @brief  是否解析完成
    int isFinished();

    /// @brief  是否解析出错
    int hasError();

    /// @brief  返回HttpRequest结构体
    HttpRequest::ptr getData() const { return m_data; }

    /// @brief  设置错误码
    void setError(int v){ m_error = v; }

    /// @brief  返回Content-Length
    uint64_t getContentLength() ;

    /// @brief  返回http_parser
    const http_parser& getParser() const { return m_parser;}

public:
    /// @brief  返回HttpRequest协议解析的缓存大小
    static uint64_t GetHttpRequestBufferSize();

    /// @brief  返回HttpRequest协议解析的最大缓存大小
    static uint64_t GetHttpRequestMaxBodySize();

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;

    // 1000:invaild method
    // 1001:invalid version 
    int m_error;
};


/// @brief  HTTP响应解析器
class HttpResponseParser{
public:
    using ptr = std::shared_ptr<HttpResponseParser>;

    /// @brief  构造函数
    HttpResponseParser();

    /// @brief          解析HTTP响应协议
    /// @param data     协议数据内存
    /// @param len      协议数据长度
    /// @param chunck   是否在解析chunck
    /// @return         返回实际解析的长度，并且将已解析的数据移除
    size_t execute(char* data, size_t len, bool chunck);

    /// @brief  是否解析完成
    int isFinished() ;

    /// @brief  是否解析出错
    int hasError() ;

    /// @brief  返回HttpResponse结构体
    HttpResponse::ptr getData() const { return m_data; }

    /// @brief  设置错误码
    void setError(int v){ m_error = v; }


    /// @brief  返回消息体长度
    uint64_t getContentLength() ; 

    /// @brief  返回http_parser
    const httpclient_parser& getParser() const { return m_parser;}
public:

    /// @brief  返回HttpResponse协议解析的缓存大小
    static uint64_t GetHttpResponseBufferSize();

    /// @brief  返回HttpResponse协议解析的最大缓存大小
    static uint64_t GetHttpResponseMaxBodySize();

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;


    int m_error;
};


}
}

