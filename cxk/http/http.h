#pragma once
#include <string>
#include <iostream>
#include <map>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include "http11_common.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace cxk{
namespace http{

/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \


/// @brief HTTP方法枚举
enum class HttpMethod{
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};


/// @brief HTTP状态码枚举
enum class HttpStatus{
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};


/// @brief      将字符串转换为HTTP方法枚举
/// @param m    HTTP方法字符串
/// @return     HTTP方法枚举
HttpMethod StringToHttpMethod(const std::string& m);


/// @brief      将字符串转换为HTTP方法枚举
/// @param m    HTTP方法字符串
/// @return     HTTP方法枚举
HttpMethod CharsTohttpMethod(const char* m);


/// @brief      将HTTP方法枚举转换为字符串
/// @param m    HTTP方法枚举
/// @return     HTTP方法字符串
const char* HttpMethodToString(const HttpMethod& m);


/// @brief      将HTTP状态码枚举转换为字符串
/// @param s    HTTP状态码枚举
/// @return     HTTP状态码字符串
const char* HttpStatusToString(const HttpStatus& s);


/// @brief  忽略大小的比较仿函数
struct CaseInsensitiveLess {
    bool operator() (const std::string& lhs, const std::string& rhs) const;
};



/// @brief      获取Map中的key值，并转成为对应类型， 返回是否成功
/// @param m    Map数据结构        
/// @param key  关键字
/// @param val  保存转换后的值
/// @param def  默认值
/// @return     @retval true  成功 @retval false 失败或者不存在
template<class MapType, class T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T()){
    auto it = m.find(key);
    if(it == m.end()){
        val = def;
        return false;
    }
    try {
        val = boost::lexical_cast<T> (it->second);
        return true;
    } catch(...){
        val = def;
    }

    return false;
}



/// @brief      获取Map的key值，并成为对应类型
/// @param m    Map数据结构
/// @param key  关键字
/// @param def  默认值
/// @return     如果存在且转换成功返回对应的值，否则返回默认值
template<class MapType, class T>
T getAs(const MapType& m, const std::string& key, const T& def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        return def;
    }

    try{
        return boost::lexical_cast<T>(it->second);
    } catch(...){

    }

    return def;
}



class HttpResponse;



/// @brief HTTP请求结构
class HttpRequest{
public:
    using ptr = std::shared_ptr<HttpRequest>;
    using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

    /// @brief          构造函数
    /// @param version  版本
    /// @param close    是否keepalive
    HttpRequest(uint8_t version = 0x11, bool close = true);   // 默认为1.1版本，并且不开启长连接

    std::shared_ptr<HttpResponse> createResponse();

    /// @brief  获取请求方法
    HttpMethod getMethod() const {return m_method;}

    /// @brief  获取HTTP版本
    uint8_t getVersion() const {return m_version;}

    /// @brief  获取HTTP状态码
    HttpStatus getStatus() const {return m_status;}

    /// @brief  获取请求路径
    const std::string& getPath() const { return m_path;}

    /// @brief  返回HTTP请求的查询参数
    const std::string& getQuery() const { return m_query;}

    /// @brief  返回HTTP请求的消息体
    const std::string& getBody() const { return m_body;}


    /// @brief  返回HTTP请求的头部
    const MapType& getHeaders() const {return m_headers;}
    
    /// @brief  返回HTTP请求的参数MAP
    const MapType& getParams() const {return m_params;}

    /// @brief  返回HTTP请求的cookie MAP
    const MapType& getCookies() const { return m_cookies;}

    /// @brief  是否自动关闭
    bool isClose() const {return m_close;}

    /// @brief  设置请求方法
    void setMethod(HttpMethod v) { m_method = v;}

    /// @brief  设置HTTP状态码
    void setStatus(HttpStatus v) { m_status = v;}

    /// @brief  设置HTTP版本
    void setVersion(uint8_t v) { m_version = v;}

    /// @brief  设置请求路径
    void setPath(const std::string& v) { m_path = v;}
    
    /// @brief  设置请求路径的查询参数
    void setQuery(const std::string& v) { m_query = v;}

    /// @brief  设置请求的Fragment
    void setFragment(const std::string& v) { m_fragment = v;}

    /// @brief  设置请求的body
    void setBody(const std::string& v) {m_body = v;}

    /// @brief  设置HTTP请求的头部
    void setHeaders(const MapType& v) {m_headers = v;}
    
    /// @brief  设置HTTP请求的参数
    void setParams(const MapType& v) {m_params = v;}

    /// @brief  设置HTTP请求的cookie
    void setCookies(const MapType& v) {m_cookies = v;}

    /// @brief  设置是否自动关闭
    void setClose(bool v){m_close = v;} // 默认不开启长连接

    bool isWebsocket() const { return m_websocket;}

    void setWebsocket(bool v){ m_websocket = v;}

    
    /// @brief      获取HTP请求的头部参数
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值
    std::string getHeader(const std::string& key, const std::string& def = "") const;  

    /// @brief      获取HTTP请求的参数
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值
    std::string getParam(const std::string& key, const std::string& def = "") const;

    /// @brief      获取HTTP请求的cookie
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值
    std::string getCookie(const std::string& key, const std::string& def = "") const;


    /// @brief      设置HTTP请求的头部
    /// @param key  关键字
    /// @param val  值
    void setHeader(const std::string& key, const std::string& val);
    
    /// @brief      设置HTTP请求的参数
    /// @param key  关键字
    /// @param val  值
    void setParam(const std::string& key, const std::string& val);
    
    /// @brief      设置HTTP请求的cookie
    /// @param key  关键字
    /// @param val  值
    void setCookie(const std::string& key, const std::string& val);

    /// @brief      删除HTTP请求的头部参数
    /// @param key  关键字
    void delHeader(const std::string& key);

    /// @brief      删除HTTP请求的参数
    /// @param key  关键字
    void delParam(const std::string& key);

    /// @brief      删除HTTP请求的cookie
    /// @param key  关键字
    void delCookie(const std::string& key);

    /// @brief      判断HTTP请求的头部参数是否存在
    /// @param key  关键字
    /// @param val  如果存在，val非空则赋值
    /// @return     是否存在
    bool hasHeader(const std::string& key, std::string* val = nullptr);

    /// @brief      判断HTTP请求的参数是否存在
    /// @param key  关键字
    /// @param val  如果存在，val非空则赋值
    /// @return     是否存在
    bool hasParam(const std::string& key, std::string* val = nullptr);

    /// @brief      判断HTTP请求的cookie是否存在
    /// @param key  关键字
    /// @param val  如果存在，val非空则赋值
    /// @return     是否存在
    bool hasCookie(const std::string& key, std::string* val = nullptr);


    /// @brief      检查并获取HTTP请求的头部参数
    /// @tparam T   转换类型
    /// @param key  关键字
    /// @param val  返回值
    /// @param def  默认值
    /// @return     如果存在且转换成功，则返回true，否则返回false
    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()){
        return checkGetAs(m_headers, key, val, def);
    }


    /// @brief      检查并获取HTTP请求的头部参数
    /// @tparam T   转换类型
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值def
    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T()){
        return getAs(m_headers, key, def);
    }


    /// @brief      检查并获取HTTP请求的参数
    /// @tparam T   转换类型
    /// @param key  关键字
    /// @param val  返回值
    /// @param def  默认值
    /// @return     如果存在且转换成功，则返回true，否则返回false
    template<class T>
    bool checkGetParamAs(const std::string& key, T& val, const T& def = T()){
        return checkGetAs(m_params, key, val, def);
    }


    /// @brief      检查并获取HTTP请求的参数
    /// @tparam T   转换类型
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值def
    template<class T>
    T getParamAs(const std::string& key, const T& def = T()){
        return getAs(m_params, key, def);
    }


    /// @brief      检查并获取HTTP请求的cookie
    /// @tparam T   转换类型
    /// @param key  关键字
    /// @param val  返回值
    /// @param def  默认值
    /// @return     如果存在且转换成功，则返回true，否则返回false
    template<class T>
    bool checkGetCookieAs(const std::string& key, T& val, const T& def = T()){
        return checkGetAs(m_cookies, key, val, def);
    }


    /// @brief      检查并获取HTTP请求的cookie
    /// @tparam T   转换类型
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值def
    template<class T>
    T getCookieAs(const std::string& key, const T& def = T()){
        return getAs(m_cookies, key, def);
    }

    /// @brief      序列化输出到流中
    /// @param os   输出流
    /// @return     输出流
    std::ostream& dump(std::ostream& os) const;

    /// @brief      转成字符串
    /// @return     字符串
    std::string toString() const ;

private:


private:
    HttpMethod m_method;
    HttpStatus m_status;
    uint8_t m_version;
    bool m_close;         // 长连接的connection

    bool m_websocket;     // 是否是websocket

    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    std::string m_body;

    MapType m_headers;
    MapType m_params;
    MapType m_cookies;
};



/// @brief  HTTP响应结构体
class HttpResponse{
public:
    using ptr = std::shared_ptr<HttpResponse>;
    using MapType = std::map<std::string, std::string, CaseInsensitiveLess> ;


    /// @brief          构造函数
    /// @param version  HTTP协议版本
    /// @param close    是否长连接（自动关闭）
    HttpResponse(uint8_t version = 0x11, bool close = true);

    /// @brief  返回响应状态
    /// @return 请求状态
    HttpStatus getStatus() const { return m_status;}

    /// @brief  返回响应版本
    /// @return HTTP协议版本
    uint8_t getVersion() const { return m_version;}

    /// @brief  返回响应内容消息体
    /// @return 响应内容
    const std::string& getBody() const { return m_body;}

    /// @brief  返回响应原因
    const std::string& getReason() const { return m_reason;}

    /// @brief  返回响应头部
    /// @return MAP
    const MapType& getHeaders() const { return m_headers;}


    /// @brief      设置响应状态
    /// @param v    响应状态
    void setStatus(HttpStatus v) { m_status = v;}

    /// @brief      设置响应版本
    /// @param v    HTTP协议版本
    void setVersion(uint8_t v) { m_version = v;}

    /// @brief      设置响应消息体
    /// @param v    响应内容
    void setBody(const std::string& v) { m_body = v;}

    /// @brief      设置响应原因
    /// @param v    响应原因
    void setReason(const std::string& v) {m_reason = v;}

    /// @brief      设置响应头部
    /// @param v    MAP
    void setHeaders(const MapType& v){ m_headers = v;}

    /// @brief      是否长连接
    bool isClose() const { return m_close;}

    /// @brief      设置是否长连接
    void setClose(bool v){m_close = v;} 

    bool isWebsocket() const { return m_websocket;}

    void setWebsocket(bool v){ m_websocket = v;}

    /// @brief      获取响应头部
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值def
    std::string getHeader(const std::string& key, const std::string& def = "") const;

    /// @brief      设置响应头部
    /// @param key  关键字
    /// @param val  值
    void setHeader(const std::string& key, const std::string& val);

    /// @brief      删除响应头部
    /// @param key  关键字
    void delHeader(const std::string& key);


    /// @brief      检查并获取HTTP响应的头部
    /// @tparam T   值类型
    /// @param key  关键字
    /// @param val  值
    /// @param def  默认值
    /// @return     如果存在则返回true，否则返回默认值val=def
    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()){
        return checkGetAs(m_headers, key, val, def);
    }


    /// @brief      获取响应头部参数
    /// @tparam T   转换类型
    /// @param key  关键字
    /// @param def  默认值
    /// @return     如果存在则返回对应值，否则返回默认值def
    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T()){
        return getAs(m_headers, key, def);
    }


    /// @brief      序列化输出到流
    /// @param os   输出流
    /// @return     输出流
    std::ostream& dump(std::ostream& os)const;

    /// @brief      转换为字符串
    std::string toString() const ;
private:
    HttpStatus m_status;
    uint8_t m_version;
    bool m_close;

    bool m_websocket;

    std::string m_body;
    std::string m_reason;
    
    MapType m_headers;
};


/// @brief      流式输出HttpRequest
/// @param os   输出流
/// @param req  请求
/// @return     输出流
std::ostream& operator<<(std::ostream& os, const HttpRequest& req);


/// @brief      流式输出HttpResponse
/// @param os   输出流
/// @param rsp  响应
/// @return     输出流
std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);

}
}