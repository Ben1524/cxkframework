#pragma once


#include "cxk/rock/rock_stream.h"
#include "cxk/tcp_server.h"


namespace cxk{

class RockServer : public TcpServer{
public:
    using ptr = std::shared_ptr<RockServer>;

    RockServer(cxk::IOManager* worker = cxk::IOManager::GetThis(), cxk::IOManager* accept_worker = cxk::IOManager::GetThis());

protected:
    virtual void handleClient(Socket::ptr client) override;
};


}
