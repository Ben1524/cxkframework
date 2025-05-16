#include "cxk/cxk.h"

static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

const char test_http_parser[] = "POST / HTTP/1.0\r\n"
    "Host: www.baidu.com\r\n"
    "Content-Length: 10\r\n\r\n"
    "1234567890";

void test(){
    cxk::http::HttpRequestParser parser;
    std::string data(test_http_parser);
    size_t s =  parser.execute(&data[0], data.size());

    CXK_LOG_DEBUG(g_logger) << "test_http_parser:" << s << " " << parser.isFinished() << " " << parser.hasError();
    CXK_LOG_DEBUG(g_logger) << "test_http_parser:" << parser.getContentLength();

    parser.getData()->dump(std::cout);
}

const char test_http_response[] = "HTTP/1.1 403 Forbidden\r\n"
"Server: nginx/1.12.2\r\n"
"Date: Sun, 28 Jul 2024 06:07:02 GMT\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 169\r\n"
"Connection: close\r\n\r\n"
"<html>\r\n"
"<head><title>403 Forbidden</title></head>\r\n"
"<body bgcolor=\"white\">\r\n"
"<center><h1>403 Forbidden</h1></center>\r\n"
"<hr><center>nginx/1.12.2</center>\r\n"
"</body>\r\n"
"</html>";


void test_response(){
    cxk::http::HttpResponseParser parser;
    std::string data(test_http_response);

    size_t s =  parser.execute(&data[0], data.size(), true);

    CXK_LOG_DEBUG(g_logger) << "test_http_response:" << s << " " << parser.isFinished() << " " << parser.hasError();
    CXK_LOG_DEBUG(g_logger) << "test_http_response:" << parser.getContentLength();
    data.resize(data.size() - s);

    parser.getData()->dump(std::cout);
    CXK_LOG_DEBUG(g_logger) << "test_http_response:" << data;
}

int main(){
    test();

    test_response();
    return 0;
}