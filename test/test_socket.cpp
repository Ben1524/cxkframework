#include <cxk/cxk.h>

static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void test_socket(){
    cxk::IPAddress::ptr addr = cxk::Address::LookupAnyIpAddr("www.baidu.com");

    addr->setPort(80);
    cxk::Socket::ptr sock = cxk::Socket::CreateTCP(addr);
    sock->connect(addr);

    const char buf[] = "GET / HTTP/1.1\r\n\r\n";
    sock->send(buf, sizeof(buf));

    std::string str;
    str.resize(4096);
    sock->recv(&str[0], str.size());

    CXK_LOG_INFO(g_logger) << str ;
}

int main(){
    cxk::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}