#include "http_connection.h"
#include "http_parser.h"
#include "cxk/stream/zlib_stream.h"


namespace cxk{
namespace http{

static Logger::ptr g_logger = CXK_LOG_NAME("system");

std::string HttpResult::toString(){
    std::stringstream ss;
    ss << "[HttpResult result=" << result
       << " error=" << error
       << " response=" << (response ? response->toString() : "nullptr")
       << " ]";
    
    return ss.str();
}


HttpConnection::HttpConnection(Socket::ptr sock, bool owner) : SocketStream(sock, owner){

}

HttpResponse::ptr HttpConnection::recvResponse(){
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();

    std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr){
        delete[] ptr;
    });

    char* data = buffer.get();
    int offset = 0;
    do{
        int len = read(data + offset, buff_size - offset);
        if(len <= 0){
            return nullptr;
        }

        len += offset;
        data[len] = 0;
        size_t nparse = parser->execute(data + offset, len, true);
        if(parser->hasError()){
            return nullptr;
        }

        offset = len - nparse;
        if(offset == (int)buff_size){
            return nullptr;
        }

        if(parser->isFinished()){
            break;
        } 
    } while(true);

    auto& client_parser = parser->getParser();
    std::string body;

    if(client_parser.chunked){
        int len = offset;   // offset为剩余数据长度
        do{
            bool begin = true;
            do{
                if(!begin || len == 0){
                    int rt = read(data + len, buff_size - len);
                    if(rt <= 0){
                        close();
                        return nullptr;
                    }
                    len += rt;
                }
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);
                if(parser->hasError()){
                    return nullptr;
                }
                len -= nparse;
                if(len == (int)buff_size){
                    return nullptr;         // 数据读完了，但还没解析完成
                }
                begin = false;
            } while(!parser->isFinished());

            // len -= 2;

            if(client_parser.content_len + 2 <= len){
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len + 2, len - client_parser.content_len + 2);
                len -= client_parser.content_len + 2;
            } else {
                body.append(data, len);
                int left = client_parser.content_len - len + 2;
                while(left > 0){
                    int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                    if(rt <= 0){
                        return nullptr;
                    } 

                    body.append(data, rt);
                    left -= rt;
                }
                body.resize(body.size() - 2);
                len = 0; 
            }

        } while(!client_parser.chunks_done);
    }else {
        uint64_t length = parser->getContentLength();
        if(length > 0){
        body.resize(length);

        int len = 0;
        if(length >= (uint64_t)offset){
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if(length > 0){
            if(readFixSize(&body[len], length) <= 0){
                return nullptr;
            }
        }
        }
    }

    if(!body.empty()){
        auto content_encoding = parser->getData()->getHeader("content-encoding");
        CXK_LOG_DEBUG(g_logger) << "content_encoding = " << content_encoding << " size = " << body.size();
        if(strcasecmp(content_encoding.c_str(), "gzip") == 0){
            auto zs = ZlibStream::CreateGzip(false);
            zs->write(body.c_str(), body.size());
            zs->flush();
            zs->getResult().swap(body);
        } else if(strcasecmp(content_encoding.c_str(), "deflate") == 0){
            auto zs = ZlibStream::CreateDeflate(false);
            zs->write(body.c_str(), body.size());
            zs->flush();
            zs->getResult().swap(body);
        }
        parser->getData()->setBody(body);
    }

    return parser->getData();
}
    
    
int HttpConnection::sendRequest(HttpRequest::ptr rsp){
     std::stringstream ss;
     ss << *rsp;
     std::string data = ss.str();
     std::cout << ss.str() << std::endl;
     writeFixSize(data.c_str(), data.size());
     return data.size();
}


HttpResult::ptr HttpConnection::DoGet(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    Uri::ptr uri = Uri::Create(url);
    if(!uri){
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invallid url : " + url);
    }

    return DoGet(uri, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnection::DoGet(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    return DoRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}



HttpResult::ptr HttpConnection::DoPost(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    Uri::ptr uri = Uri::Create(url);
    if(!uri){
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invallid url : " + url);
    }

    return DoPost(uri, timeout_ms, headers, body);
}



HttpResult::ptr HttpConnection::DoPost(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    return DoRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    Uri::ptr uri = Uri::Create(url);
    if(!uri){
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invallid url : " + url);
    }

    return DoRequest(method, uri, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url->getPath());
    req->setQuery(url->getQuery());
    req->setFragment(url->getFragment());
    req->setMethod(method);
    bool has_host = false;

    for(auto& i : headers){
        if(strcasecmp(i.first.c_str(), "connection") == 0){
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0){
                req->setClose(false);
            }
            continue;
        }

        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0){
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }

    if(!has_host){
        req->setHeader("Host", url->getHost());
    }

    req->setBody(body);
    return DoRequest(req, url, timeout_ms);
}
    
  
  
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr url, uint64_t timeout_ms){
    bool is_ssl = url->getScheme() == "https";
    Address::ptr addr = url->createAddress();
    if(!addr){
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr, "invallid host : " + url->getHost());
    }
    // Socket::ptr sock = Socket::CreateTCP(addr); // 根据地址创建tcp
    Socket::ptr sock = is_ssl ? Socket::CreateTCP(addr) : Socket::CreateTCP(addr);
    if(!sock){
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR, nullptr, "connect failed: " + addr->toString());
    }
    if(!sock->connect(addr)){
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FALT, nullptr, "connect failed: " + addr->toString());
    }

    sock->setRecvTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int ret = conn->sendRequest(req);
    if(ret == 0){
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send request closed by peer");
    }

    if(ret < 0){
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send request socket error errno="+std::to_string(errno));
    }

    auto rsp = conn->recvResponse();
    if(!rsp){
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv response timemout: "+std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}


HttpConnection::~HttpConnection(){
    CXK_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
}



HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string& uri, const std::string& vhost, uint32_t max_size, 
    uint32_t max_alive_time, uint32_t max_request){
    Uri::ptr turi = Uri::Create(uri);
    if(!turi){
        CXK_LOG_ERROR(g_logger) << "invalid uri = " << uri;
    }

    return std::make_shared<HttpConnectionPool>(turi->getHost(), vhost, turi->getPort(), turi->getScheme() == "https", 
        max_size, max_alive_time, max_request);
}



HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost,uint32_t port, bool is_https,  uint32_t maxSize, uint32_t max_alive_time, 
        uint32_t max_request) :
    m_host(host),
    m_vhost(vhost), 
    m_port(port ? port : (is_https ? 443 : 80)), 
    m_maxSize(maxSize), 
    m_maxAliveTime(max_alive_time), 
    m_maxRequest(max_request),
    m_isHttps(is_https){
}

HttpConnection::ptr HttpConnectionPool::getConnection(){
    uint64_t now_ms = cxk::GetCurrentMS();
    std::vector<HttpConnection*> invalid_conn;  // 存放无效连接
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    while(!m_conns.empty()){
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        if(!conn->isConnected()){
            invalid_conn.push_back(conn);

            continue;
        }
        if(conn->m_createTime + m_maxAliveTime > now_ms){
            // 链接失效
            invalid_conn.push_back(conn);
            continue;
        }
        ptr = conn;
        break;
    }
    lock.unlock();
    for(auto i : invalid_conn){
        delete i;
    }

    m_total -= invalid_conn.size();

    if(!ptr){
        IPAddress::ptr addr = Address::LookupAnyIpAddr(m_host);
        if(!addr){
            CXK_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        Socket::ptr sock = m_isHttps ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        addr->setPort(m_port);
        if(!sock){
            CXK_LOG_ERROR(g_logger) << "create sock fail" << * addr;
            return nullptr;
        }

        if(!sock->connect(addr)){
            CXK_LOG_ERROR(g_logger) << "sock connect fail" << *addr;
            return nullptr;
        }

        ptr = new HttpConnection(sock);
        ++m_total;
    }

    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
}    



HttpResult::ptr HttpConnectionPool::doGet(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    std::stringstream ss;
    ss << url->getPath()
       << (url->getQuery().empty() ? "" : "?")
       << url->getQuery()
       << (url->getFragment().empty() ? "" : "#")
       << url->getFragment();
    
    return doGet(ss.str(), timeout_ms, headers, body);
}




HttpResult::ptr HttpConnectionPool::doPost(const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){

    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    std::stringstream ss;
    ss << url->getPath()
       << (url->getQuery().empty() ? "" : "?")
       << url->getQuery()
       << (url->getFragment().empty() ? "" : "#")
       << url->getFragment();
    
    return doPost(ss.str(), timeout_ms, headers, body);
}




HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){

    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;

    for(auto& i : headers){
        if(strcasecmp(i.first.c_str(), "connection") == 0){
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0){
                req->setClose(false);
            }
            continue;
        }

        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0){
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }

    if(!has_host){
        if(m_vhost.empty()){
            req->setHeader("Host", m_host);
        } else{
            req->setHeader("Host", m_vhost);
        }
    }

    req->setBody(body);
    return doRequest(req, timeout_ms);

}



HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, Uri::ptr url, uint64_t timeout_ms, 
        const std::map<std::string, std::string>& headers, const std::string& body){
    std::stringstream ss;
    ss << url->getPath()
       << (url->getQuery().empty() ? "" : "?")
       << url->getQuery()
       << (url->getFragment().empty() ? "" : "#")
       << url->getFragment();
    
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}
    


HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout_ms){   
    auto conn = getConnection();
    if(!conn){
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION, nullptr, "connect failed: " + m_host);
    }

    Socket::ptr sock = conn->getSocket();
    if(!sock){
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION, nullptr, "connect failed: " + m_host);
    }
    sock->setRecvTimeout(timeout_ms);
    int ret = conn->sendRequest(req);
    if(ret == 0){
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send request closed by peer");
    }

    if(ret < 0){
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr, "send request socket error errno="+std::to_string(errno));
    }

    auto rsp = conn->recvResponse();
    if(!rsp){
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr, "recv response timemout: "+std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}


void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool){
    ++ptr->m_request;
    if(!ptr->isConnected() || ptr->m_createTime + pool->m_maxAliveTime >= cxk::GetCurrentMS() || ptr->m_request >= pool->m_maxRequest){
        delete ptr;
        --pool->m_total;
        return ;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
} 

}


}