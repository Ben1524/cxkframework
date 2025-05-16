#include <iostream>
#include "cxk/cxk.h"

static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();


void test_pool(){
    cxk::http::HttpConnectionPool::ptr pool(new cxk::http::HttpConnectionPool("www.baidu.com", "", 80, false, 10, 1000 * 30, 20));
    // 添加一个循环定时器
    cxk::IOManager::GetThis()->addTimer(1000, [pool](){
        auto r =  pool->doGet("/", 300);
        CXK_LOG_INFO(g_logger) << r->toString();
    }, true);
}





void test_https() {
    auto r = cxk::http::HttpConnection::DoGet("http://www.baidu.com/", 300, {
                        {"Accept-Encoding", "gzip, deflate, br"},
                        {"Connection", "keep-alive"},
                        {"User-Agent", "curl/7.29.0"}
            });
    CXK_LOG_INFO(g_logger) << "result=" << r->result
        << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");

    auto pool = cxk::http::HttpConnectionPool::Create(
                    "https://www.baidu.com", "", 10, 1000 * 30, 5);
    cxk::IOManager::GetThis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 3000, {
                        {"Accept-Encoding", "gzip, deflate, br"},
                        {"User-Agent", "curl/7.29.0"}
                    });
            CXK_LOG_INFO(g_logger) << r->toString();
    }, true);
}


void run(){
    cxk::Address::ptr addr = cxk::Address::LookupAnyIpAddr("www.sylar.top:80");
    if(!addr){
        CXK_LOG_ERROR(g_logger) << "lookup address error";
        return ;
    }

    cxk::Socket::ptr sock = cxk::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt){
        CXK_LOG_ERROR(g_logger) << "connect error";
        return ;
    }

    cxk::http::HttpConnection::ptr conn(new cxk::http::HttpConnection(sock));
    cxk::http::HttpRequest::ptr req(new cxk::http::HttpRequest());
    req->setPath("/blog/");
    req->setHeader("Host", "www.sylar.top");



    auto rsp = conn->sendRequest(req);
    CXK_LOG_DEBUG(g_logger) << "hello";
    auto resp = conn->recvResponse();
    CXK_LOG_DEBUG(g_logger) << "world";

    if(!rsp){
        CXK_LOG_ERROR(g_logger) << "send request error";
        return ;
    }
    CXK_LOG_INFO(g_logger) << *resp;

    CXK_LOG_INFO(g_logger) << "===========================================" << std::endl;

    auto ret = cxk::http::HttpConnection::DoGet("http://www.sylar.top/", 300);
    CXK_LOG_INFO(g_logger) << "result: " << ret->result<<" error: " << ret->error << " rsp=" << (rsp ? ret->response->toString() : "");


    CXK_LOG_INFO(g_logger) << "===========================================" << std::endl;
    test_pool();
}



int main(){

    cxk::IOManager iom(2);
    iom.schedule(test_https);
    return 0;
}