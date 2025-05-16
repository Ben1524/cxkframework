#include "cxk/http/http.h"

void test_req(){
    cxk::http::HttpRequest::ptr req(new cxk::http::HttpRequest());
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello world");

    req->dump(std::cout) << std::endl;
}


void test_resp(){
    cxk::http::HttpResponse::ptr resp(new cxk::http::HttpResponse());
    resp->setHeader("X-X", "cxk");
    resp->setBody("hello world");
    resp->setStatus((cxk::http::HttpStatus)400);
    resp->dump(std::cout) << std::endl;
}

int main(){
    test_resp();

    return 0;
}