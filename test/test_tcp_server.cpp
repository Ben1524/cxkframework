#include "cxk/cxk.h"

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void run(){
    auto addr = cxk::Address::LookupAny("0.0.0.0:8033");

    auto addr2 = cxk::UnixAddress::ptr(new cxk::UnixAddress("/tmp/unix_addr"));

    CXK_LOG_INFO(g_logger) << (*addr) << (*addr2);

    std::vector<cxk::Address::ptr> addrs;
    std::vector<cxk::Address::ptr> fails;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    cxk::TcpServer::ptr server (new cxk::TcpServer());
    while(!server->bind(addrs, fails)){
        sleep(2);
    }
    server->start();
}


int main(){
    cxk::IOManager iom(2);

    iom.schedule(run);

    return 0;
}