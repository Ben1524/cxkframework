#pragma once

#include "ws_servlet.h"
#include "ws_session.h"
#include "cxk/tcp_server.h"

namespace cxk{
namespace http{

class WSServer : public TcpServer{
public:
    using ptr = std::shared_ptr<WSServer> ;

    WSServer(cxk::IOManager* worker = cxk::IOManager::GetThis(), 
        cxk::IOManager* accept_worker = cxk::IOManager::GetThis());

    WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch; }

    void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v; }

protected:
    virtual void handleClient(Socket::ptr client) override;

protected:
    WSServletDispatch::ptr m_dispatch;
};



}


}