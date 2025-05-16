#pragma once
#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"


namespace cxk{


/// @brief URI 统一资源标识符
class Uri{
public:
    using ptr = std::shared_ptr<Uri>;

    static Uri::ptr Create(const std::string& uri);
    Uri();

    const std::string& getScheme() const { return m_scheme; }
    const std::string& getUserinfo() const { return m_userinfo; }
    const std::string& getHost() const { return m_host; }
    const std::string& getPath() const;// { return m_path; }
    const std::string& getQuery() const { return m_query; }
    const std::string& getFragment() const { return m_fragment; }
    int32_t getPort() const ;
    void setScheme(const std::string& scheme) { m_scheme = scheme; }
    void setUserinfo(const std::string& userinfo) { m_userinfo = userinfo; }
    void setHost(const std::string& host) { m_host = host; }
    void setPort(uint32_t port) { m_port = port; }
    void setPath(const std::string& path) { m_path = path; }
    void setQuery(const std::string& query) { m_query = query; }
    void setFragment(const std::string& fragment) { m_fragment = fragment; }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    /// @brief 根据uri创建一个地址
    /// @return 创建的地址
    Address::ptr createAddress() const;   

private:
    bool isDefaultPort() const;
private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    uint32_t m_port;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
};


}