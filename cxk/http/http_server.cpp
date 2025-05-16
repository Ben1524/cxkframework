#include "http_server.h"

namespace cxk{
namespace http{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

HttpServer::HttpServer(bool isKeepAlive , cxk::IOManager* worker, 
        cxk::IOManager* accept_worker) : TcpServer(worker, accept_worker) ,m_isKeepAlive(isKeepAlive){
    m_dispatch.reset(new ServletDispatch());
    m_type = "http";
}


void HttpServer::setName(const std::string& v){
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}


void HttpServer::handleClient(Socket::ptr client){
    CXK_LOG_DEBUG(g_logger) << "new client ";
    cxk::http::HttpSession::ptr session (new HttpSession(client));
    do{
        auto req = session->recvRequest();
        if(!req){
            CXK_LOG_WARN(g_logger)  << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno);
            break;
        }


        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepAlive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);


        CXK_LOG_DEBUG(g_logger) << "request: " << std::endl
            << * req;
        CXK_LOG_DEBUG(g_logger) << "response: " << std::endl
            << * rsp;

        session->sendResponse(rsp);

        if(!m_isKeepAlive || req->isClose()){
            break;
        }
    } while(true);
    session->close();
}


}


}