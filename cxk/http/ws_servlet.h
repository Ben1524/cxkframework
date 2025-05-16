#pragma once

#include "ws_session.h"
#include "cxk/Thread.h"
#include "servlet.h"

namespace cxk{
namespace http{

class WSServlet : public Servlet{
public:
    using ptr = std::shared_ptr<WSServlet>;
    WSServlet(const std::string& name):Servlet(name){

    }

    virtual int32_t handle(cxk::http::HttpRequest::ptr request
                   , cxk::http::HttpResponse::ptr response
                   , cxk::http::HttpSession::ptr session) override {
        return 0;
    }

    virtual int32_t onConnect(cxk::http::HttpRequest::ptr header, cxk::http::WSSession::ptr session) = 0;

    virtual int32_t onClose(cxk::http::HttpRequest::ptr header, cxk::http::WSSession::ptr session) = 0;

    virtual int32_t handle(cxk::http::HttpRequest::ptr header, cxk::http::WSFrameMessage::ptr msg, 
        cxk::http::WSSession::ptr session) = 0;

    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
};



class FunctionWSServlet : public WSServlet{
public:
    using ptr = std::shared_ptr<FunctionWSServlet>;
    using on_connect_cb = std::function<int32_t(HttpRequest::ptr header, WSSession::ptr session)>;
    using on_close_cb = std::function<int32_t(HttpRequest::ptr header, WSSession::ptr session)>;
    using callback = std::function<int32_t(HttpRequest::ptr header, WSFrameMessage::ptr msg, 
        WSSession::ptr session)>;

    FunctionWSServlet(callback cb, on_connect_cb connect_cb = nullptr, on_close_cb close_cb = nullptr);

    virtual int32_t onConnect(HttpRequest::ptr header, WSSession::ptr session) override;

    virtual int32_t onClose(HttpRequest::ptr header, WSSession::ptr session) override;

    virtual int32_t handle(HttpRequest::ptr header, WSFrameMessage::ptr msg, 
        WSSession::ptr session) override;

protected:
    callback m_callback;
    on_connect_cb m_onConnect;
    on_close_cb m_onClose;
};

class WSServletDispatch : public ServletDispatch{
public:
    using ptr = std::shared_ptr<WSServletDispatch>;
    using RWMutexType = RWMutex;
    
    WSServletDispatch();

    void addServlet(const std::string& uri, FunctionWSServlet::callback cb, 
        FunctionWSServlet::on_connect_cb connect_cb = nullptr, 
        FunctionWSServlet::on_close_cb close_cb = nullptr);

    void addGlobServlet(const std::string& uri, FunctionWSServlet::callback cb, 
        FunctionWSServlet::on_connect_cb connect_cb = nullptr, 
        FunctionWSServlet::on_close_cb close_cb = nullptr);

    WSServlet::ptr getWSServlet(const std::string& uri);
};




}

}