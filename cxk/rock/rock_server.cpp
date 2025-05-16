#include "rock_server.h"
#include "cxk/logger.h"
#include "cxk/module.h"

namespace cxk{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");


RockServer::RockServer(cxk::IOManager* worker, cxk::IOManager* accept_worker) : TcpServer(worker, accept_worker) {
    m_type = "rock";
}



void RockServer::handleClient(Socket::ptr client){
    CXK_LOG_DEBUG(g_logger) << "handle Client ";
    cxk::RockSession::ptr session(new cxk::RockSession(client));
    
    ModuleMgr::GetInstance()->foreach(Module::ROCK, [session](Module::ptr m){
        m->onConnect(session);
    });

    session->setDisconnectCb([](AsyncSocketStream::ptr stream){
        ModuleMgr::GetInstance()->foreach(Module::ROCK, [stream](Module::ptr m){
            m->onDisconnect(stream);
        });
    });

    session->setRequestHandler([](cxk::RockRequest::ptr req, cxk::RockResponse::ptr rsp, cxk::RockStream::ptr conn)->bool{
        CXK_LOG_INFO(g_logger) << "handleReq" << req->toString() << " body = " << req->getBody();
        bool rt = false;
        ModuleMgr::GetInstance()->foreach(Module::ROCK, [&rt, req, rsp, conn](Module::ptr m){
            if(rt){
                return ;
            }
            rt = m->handleRequest(req, rsp, conn);
        });
        return rt;
    });


    session->setNotifyHandler([](cxk::RockNotify::ptr nty, cxk::RockStream::ptr conn)->bool{
        CXK_LOG_INFO(g_logger) << "handleNty" << nty->toString();
        bool rt = false;
        ModuleMgr::GetInstance()->foreach(Module::ROCK, [&rt, nty, conn](Module::ptr m){
            if(rt){
                return;
            }
            rt = m->handleNotify(nty, conn);
        });
        return rt;
    });

    session->start();
}


}