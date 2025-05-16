#pragma once

#include "cxk/stream/socket_stream.h"
#include "http.h"
#include "http_parser.h"
#include "cxk/uri.h"
#include <cxk/Thread.h>

namespace cxk{
namespace http{

/// @brief HTTP响应结果
struct HttpResult{
    using ptr = std::shared_ptr<HttpResult>;

    enum class Error{
        OK = 0,
        INVALID_URL = 1,
        INVALID_HOST = 2,
        CONNECT_FALT = 3,
        SEND_CLOSE_BY_PEER = 4,
        SEND_SOCKET_ERROR = 5,
        TIMEOUT = 6,
        CREATE_SOCKET_ERROR = 7,
        POOL_GET_CONNECTION = 8,
        POOL_INVALID_CONNECTION = 9,
    };

    HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error): 
        result(_result), response(_response), error(_error){
    }
    int result;
    HttpResponse::ptr response;
    std::string  error;

    std::string toString();
};


/// @brief HTTP 客户端类
class HttpConnection : public SocketStream{
friend class HttpConnectionPool;
public:
    using ptr = std::shared_ptr<HttpConnection>;

    /// @brief 发送GET请求
    /// @param url URL
    /// @param timeout_ms   超时时间 
    /// @param headers  HTTP请求头
    /// @param body     HTTP请求体
    /// @return     HTTP响应结果
    static HttpResult::ptr DoGet(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /// @brief 发送POST请求
    /// @param url URL
    /// @param timeout_ms   超时时间 
    /// @param headers  HTTP请求头
    /// @param body     HTTP请求体
    /// @return     HTTP响应结果
    static HttpResult::ptr DoPost(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr DoPost(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /// @brief 发送自定义请求
    /// @param method 请求方法
    /// @param url  URL
    /// @param timeout_ms   超时时间 
    /// @param headers      HTTP请求头
    /// @param body         HTTP请求体
    /// @return     HTTP响应结果
    static HttpResult::ptr DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    
    static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr url, uint64_t timeout_ms);

    /// @brief 析构函数
    ~HttpConnection();

    /// @brief 构造函数
    /// @param sock  Socket类
    /// @param owner 是否掌握所有权 
    HttpConnection(Socket::ptr sock, bool owner = true);

    /// @brief 接受http响应
    HttpResponse::ptr recvResponse();

    /// @brief 发送http请求
    /// @param req HTTP请求结构
    /// @return 是否成功
    int sendRequest(HttpRequest::ptr req);

private:
    uint64_t m_createTime = 0;
    uint64_t m_request = 0;

};

class HttpConnectionPool{
public:
    using ptr = std::shared_ptr<HttpConnectionPool>;
    using MutexType = Mutex;


    static HttpConnectionPool::ptr Create(const std::string& uri, const std::string& vhost, uint32_t max_size, uint32_t max_alive_time, 
        uint32_t max_requst);


    HttpConnectionPool(const std::string& host, const std::string& vhost,uint32_t port,bool is_https, uint32_t maxSize, uint32_t max_alive_time, 
        uint32_t max_request);

    /// @brief 从连接池取出一个http连接
    /// @return http连接
    HttpConnection::ptr getConnection();    

    /// @brief 发送GET请求
    /// @param url URL
    /// @param timeout_ms   超时时间 
    /// @param headers  HTTP请求头
    /// @param body     HTTP请求体
    /// @return     HTTP响应结果
    HttpResult::ptr doGet(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");


    HttpResult::ptr doGet(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    /// @brief 发送POST请求
    /// @param url URL
    /// @param timeout_ms   超时时间 
    /// @param headers  HTTP请求头
    /// @param body     HTTP请求体
    /// @return     HTTP响应结果
    HttpResult::ptr doPost(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");


    HttpResult::ptr doPost(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");


    /// @brief 发送自定义请求
    /// @param method 请求方法
    /// @param url  URL
    /// @param timeout_ms   超时时间 
    /// @param headers      HTTP请求头
    /// @param body         HTTP请求体
    /// @return     HTTP响应结果
    HttpResult::ptr doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method, Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
    
    HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);

private:
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool); 

private:
    std::string m_host;
    std::string m_vhost;
    uint32_t m_port;
    uint32_t m_maxSize;          // 最多的连接数
    uint32_t m_maxAliveTime;     // 连接最大存活时间
    uint32_t m_maxRequest;       // 最大请求数
    bool m_isHttps;

    MutexType m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};
};



}





}