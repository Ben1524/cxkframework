#pragma once
#include "cxk/tcp_server.h"
#include "http_session.h"
#include "servlet.h"


namespace cxk{
namespace http{


/// @brief HTTP服务器类
class HttpServer : public TcpServer{
public:
    using ptr = std::shared_ptr<HttpServer>;
    

    /// @brief                  构造函数
    /// @param isKeepAlive      是否保持连接
    /// @param worker           工作调度器
    /// @param accept_worker    接受链接调度器
    HttpServer(bool isKeepAlive = false, cxk::IOManager* worker = cxk::IOManager::GetThis(), 
        cxk::IOManager* accept_worker = cxk::IOManager::GetThis());


    /// @brief  获取servletDispatch
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

    /// @brief  设置servletDispatch
    void setServletDispatch(ServletDispatch::ptr dispatch) { m_dispatch = dispatch; }

    virtual void setName(const std::string& v) override;

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    bool m_isKeepAlive;
    ServletDispatch::ptr m_dispatch;

};


}


}