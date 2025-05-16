#include "cxk/http/ws_server.h"
#include "cxk/logger.h"



static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();


void run(){
    cxk::http::WSServer::ptr server(new cxk::http::WSServer);
    cxk::Address::ptr addr = cxk::Address::LookupAnyIpAddr("0.0.0.0:8020");
    if(!addr){
        CXK_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    auto fun = [](cxk::http::HttpRequest::ptr header, 
        cxk::http::WSFrameMessage::ptr msg, cxk::http::WSSession::ptr session){
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addServlet("/cxk", fun);
    while(!server->bind(addr)){
        CXK_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}


int main(){
    cxk::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

