#include "ws_server.h"
#include "cxk/logger.h"

namespace cxk{
namespace http{


static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");


WSServer::WSServer(cxk::IOManager* worker , cxk::IOManager* accept_worker) : TcpServer(worker, accept_worker){
    m_dispatch.reset(new WSServletDispatch);
    m_type = "websocket_server";
}

 
 
void WSServer::handleClient(Socket::ptr client) {
    CXK_LOG_DEBUG(g_logger) << "handleClient" ;

    WSSession::ptr session(new WSSession(client));
    do{
        HttpRequest::ptr header = session->handleShake();
        if(!header){
            CXK_LOG_DEBUG(g_logger) << "handleShake error";
            break;
        }

        WSServlet::ptr servlet = m_dispatch->getWSServlet(header->getPath());
        if(!servlet){
            CXK_LOG_DEBUG(g_logger) << "no match WSServlet";
            break;
        }

        int rt = servlet->onConnect(header, session);
        if(rt){
            CXK_LOG_DEBUG(g_logger) << "onConnect return " << rt;
            break;
        }
        while(true){
            auto msg = session->recvMessage();
            if(!msg){
                break;
            }
            rt = servlet->handle(header, msg, session);
            if(rt){
                CXK_LOG_DEBUG(g_logger) << "handle return " << rt;
                break;
            }
        }
        servlet->onClose(header, session);
    }while(0);
    session->close();
}


}
}