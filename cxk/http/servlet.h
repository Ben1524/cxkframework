#pragma once
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include "cxk/Thread.h"
#include <unordered_map>
#include "http.h"
#include "http_session.h"

namespace cxk{

namespace http{

/// @brief Servlet 接口类
class Servlet{
public:
    using ptr = std::shared_ptr<Servlet>;

    /// @brief          构造函数
    /// @param name     servlet 名称
    Servlet(const std::string& name): m_name(name){}

    /// @brief          析构函数
    virtual ~Servlet(){}

    /// @brief          处理请求
    /// @param request  请求
    /// @param response 响应
    /// @param session  HTTP连接会话
    /// @return         是否处理成功
    virtual int32_t handle(cxk::http::HttpRequest::ptr request,
        cxk::http::HttpResponse::ptr response, cxk::http::HttpSession::ptr session) = 0;

    /// @brief          获取名称
    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
};


/// @brief          函数式Servlet
class FunctionServlet : public Servlet{
public:
    using ptr = std::shared_ptr<FunctionServlet>;
    using callback = std::function<int32_t (cxk::http::HttpRequest::ptr request, cxk::http::HttpResponse::ptr response, 
            cxk::http::HttpSession::ptr session)>;

    /// @brief          构造函数
    /// @param cb       回调函数
    FunctionServlet(callback cb);

    virtual int32_t handle(cxk::http::HttpRequest::ptr request,
        cxk::http::HttpResponse::ptr response, cxk::http::HttpSession::ptr session) override;
private:
    callback m_cb;

};




/// @brief Servlet分发器
class ServletDispatch : public Servlet{
public:
    using ptr = std::shared_ptr<ServletDispatch>;
    using RWMutexType = cxk::RWMutex;

    /// @brief          构造函数
    ServletDispatch();


    virtual int32_t handle(cxk::http::HttpRequest::ptr request,
        cxk::http::HttpResponse::ptr response, cxk::http::HttpSession::ptr session) override;


    /// @brief          添加Servlet
    /// @param uri      uri
    /// @param slt      servlet
    void addServlet(const std::string& uri, Servlet::ptr slt);

    /// @brief          添加Servlet
    /// @param uri      uri
    /// @param cb       FunctionServlet回调函数
    void addServlet(const std::string& uri, FunctionServlet::callback cb);


    /// @brief          添加模糊匹配的Servlet
    /// @param uri      uri
    /// @param slt      servlet
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);

    /// @brief          添加模糊匹配的Servlet
    /// @param uri      uri
    /// @param cb       FunctionServlet回调函数
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);


    /// @brief          删除Servlet
    void delServlet(const std::string& uri);

    /// @brief          删除模糊匹配的Servlet
    void delGlobServlet(const std::string& uri);


    /// @brief          获取默认的servlet
    Servlet::ptr getDefault() const { return m_default; }       // 有线程安全的问题
  
    /// @brief          设置默认的servlet
    void setDefault(Servlet::ptr slt) { m_default = slt; }

    /// @brief          根据uri获取servlet
    /// @param uri      uri
    /// @return         uri对应的servlet
    Servlet::ptr getServlet(const std::string& uri);

    /// @brief          根据uri获取模糊匹配的servlet
    /// @param uri      uri
    /// @return         模糊匹配的servlet
    Servlet::ptr getGlobServlet(const std::string& uri);

    /// @brief          根据uri获取匹配的servlet
    /// @param uri      uri
    /// @return         优先精准匹配，其次进行模糊匹配，最后是默认的servlet
    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    RWMutexType m_mutex;
    // uri -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // 模糊匹配
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;

    Servlet::ptr m_default; // 默认的servlet， 所有的路径都无法匹配时
};


/// @brief 404 Not Found Servlet
class NotFoundServlet : public Servlet{
public:
    using ptr = std::shared_ptr<NotFoundServlet>;

    NotFoundServlet(const std::string& name);

    virtual int32_t handle(cxk::http::HttpRequest::ptr request,
        cxk::http::HttpResponse::ptr response, cxk::http::HttpSession::ptr session) override;
private:
    std::string m_name;
    std::string m_content;
};



}
}