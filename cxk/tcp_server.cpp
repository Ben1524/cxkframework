#include "tcp_server.h"
#include "config.h"
#include "logger.h"

namespace cxk{

static cxk::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = cxk::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

TcpServer::TcpServer(IOManager* worker,  IOManager* acceptWorker) : m_worker(worker),
        m_acceptWorker(acceptWorker) , m_readTimeOut(g_tcp_server_read_timeout->getValue()), m_name("cxk/1.0.0"), m_isStop(true){

}

TcpServer::~TcpServer(){
    for(auto& i : m_socks){
        i->close();
    }
    m_socks.clear();
}

void TcpServer::setConf(const TcpServerConf& v) {
    m_conf.reset(new TcpServerConf(v));
}

bool TcpServer::bind(cxk::Address::ptr addr, bool ssl){
    std::vector<cxk::Address::ptr> addrs;
    std::vector<cxk::Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}


bool TcpServer::bind(const std::vector<cxk::Address::ptr>& addrs, std::vector<cxk::Address::ptr> fails, bool ssl){
    m_ssl = ssl;
    for(auto& addr : addrs){
        Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
        if(!sock->bind(addr)){
            CXK_LOG_ERROR(g_logger) << "bind " << addr->toString() << " failed";
            fails.push_back(addr);
            continue;
        }

        if(!sock->listen()){
            CXK_LOG_ERROR(g_logger) << "listen " << addr->toString() << " failed";
            fails.push_back(addr);
            continue;
        }

        m_socks.push_back(sock);
    }

    if(!fails.empty()){
        m_socks.clear();
        return false;
    }

    for(auto& i : m_socks){
        CXK_LOG_INFO(g_logger)  << "type=" << m_type
            << " name=" << m_name
            << " ssl=" << m_ssl
            << " server bind success: " << i;
    }
    return true;
}


bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file){
    for(auto& i : m_socks){
        auto ssl_socket = std::dynamic_pointer_cast<SSLSocket>(i);
        if(ssl_socket){
            if(!ssl_socket->loadCertificates(cert_file, key_file)){
                return false;
            }
        }
    } 
    return true;
}

bool TcpServer::start(){  
    if(!m_isStop){
        return true;
    }

    m_isStop = false;
    for(auto& i : m_socks){
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), i));
    }

    return true;
}


void TcpServer::stop(){
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self](){
        for(auto& i : m_socks){
            i->cancelAll();
            i->close();
        }
        m_socks.clear();
    });

}


void TcpServer::handleClient(Socket::ptr client){
    CXK_LOG_INFO(g_logger) << "handleClient";
}



void TcpServer::startAccept(Socket::ptr sock){
    while(!m_isStop){
        Socket::ptr client = sock->accept();
        if(client){
            client->setRecvTimeout(m_readTimeOut);
            m_worker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        } else {
            CXK_LOG_ERROR(g_logger) << "accept failed";
        }

    }
}

}