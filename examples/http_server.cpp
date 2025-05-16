#include "cxk/http/http_server.h"
#include "cxk/logger.h"

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();
cxk::IOManager::ptr worker;


void run(){
    g_logger->setLogLevel(cxk::LogLevel::DEBUG);
    cxk::Address::ptr addr = cxk::Address::LookupAnyIpAddr("0.0.0.0:8020");
    if(!addr){
        CXK_LOG_ERROR(g_logger) << "lookup any ip addr failed";
        return;
    }

    cxk::http::HttpServer::ptr http_server(new cxk::http::HttpServer(true, worker.get()));

    bool ssl = false;
    while(!http_server->bind(addr, ssl)){
        CXK_LOG_ERROR(g_logger) << "bind failed";
        sleep(1);
    }

    if(ssl){
        http_server->loadCertificates("/home/cxk/work/web-serve/keys/server.crt", "/home/cxk/work/web-serve/keys/server.key");
    }

    http_server->start();
}


int main(int argc, char *argv[]){
    cxk::IOManager iom(1);
    worker.reset(new cxk::IOManager(4, false));
    iom.schedule(run);

    return 0;
}

