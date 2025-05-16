#pragma once
#include <memory>
#include <functional>
#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "noncopyable.h"
#include "config.h"


namespace cxk{


struct TcpServerConf{
    using ptr = std::shared_ptr<TcpServerConf>;
    
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 1000 * 2 * 60;
    int ssl = 0;
    std::string id;

    std::string type = "http";
    std::string name;
    std::string cert_file;
    std::string key_file;
    std::string accept_worker;
    std::string process_worker;
    std::map<std::string, std::string> args;

    bool isValid() const{
        return !address.empty();
    }


    bool operator==(const TcpServerConf& conf) const{
        return address == conf.address &&
            keepalive == conf.keepalive &&
            timeout == conf.timeout && 
            name == conf.name &&
            ssl == conf.ssl &&
            cert_file == conf.cert_file &&
            key_file == conf.key_file &&
            accept_worker == conf.accept_worker &&
            process_worker == conf.process_worker &&
            args == conf.args &&
            id == conf.id &&
            type == conf.type;
    }
};
    
    
template<>
class LexicalCast<std::string, TcpServerConf>{
public:
TcpServerConf operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        TcpServerConf conf;
        conf.id = node["id"].as<std::string>(conf.id);
        conf.type = node["type"].as<std::string>(conf.type);
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        conf.ssl = node["ssl"].as<int>(conf.ssl);
        conf.cert_file = node["cert_file"].as<std::string>(conf.cert_file);
        conf.key_file = node["key_file"].as<std::string>(conf.key_file);
        conf.accept_worker = node["accept_worker"].as<std::string>();
        conf.process_worker = node["process_worker"].as<std::string>();
        conf.args = LexicalCast<std::string, std::map<std::string, std::string>>()(node["args"].as<std::string>(""));

        if(node["address"].IsDefined()){
            for(size_t i = 0; i < node["address"].size(); ++i){
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }

        return conf;
    }
};


template<>
class LexicalCast<TcpServerConf, std::string>{
public:
    std::string operator()(const TcpServerConf& conf){
        YAML::Node node;
        node["id"] = conf.id;
        node["type"] = conf.type;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["ssl"] = conf.ssl;
        node["cert_file"] = conf.cert_file;
        node["key_file"] = conf.key_file;
        node["accept_worker"] = conf.accept_worker;
        node["process_worker"] = conf.process_worker;
        node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>, std::string>()(conf.args));
        for(auto& i : conf.address){
            node["address"].push_back(i);
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};






/// @brief  TCP服务器封装
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable{
public:
    using ptr = std::shared_ptr<TcpServer>;

    /// @brief              构造函数
    /// @param worker       socket客户端工作的协程调度器
    /// @param acceptWorker 服务器socket接受socket连接的协程调度器
    TcpServer(IOManager* worker = cxk::IOManager::GetThis(), 
    IOManager* acceptWorker = cxk::IOManager::GetThis());

    /// @brief              析构函数
    virtual ~TcpServer();


    /// @brief          绑定地址
    /// @return          true 绑定成功 false 绑定失败
    virtual bool bind(cxk::Address::ptr addr, bool ssl = false);

    /// @brief          绑定地址
    /// @param addr     要绑定的地址数组
    /// @param          绑定失败的地址数组
    /// @return         是否绑定成功
    virtual bool bind(const std::vector<cxk::Address::ptr>& addr, std::vector<cxk::Address::ptr>, bool ssl = false);


    bool loadCertificates(const std::string& cert_file, const std::string& key_file);

    /// @brief      启动服务
    /// @pre        bind必须先调用
    virtual bool start();

    /// @brief      停止服务
    virtual void stop();


    /// @brief      获取读超时时间
    uint64_t getReadTimeOut() const { return m_readTimeOut; }
    
    /// @brief      获取服务器名称
    virtual std::string getName() const { return m_name; }


    /// @brief      设置读超时时间
    void setReadTimeOut(uint64_t time) { m_readTimeOut = time; }
    
    /// @brief      设置服务器名称
    virtual void setName(const std::string& name) { this->m_name = name; }

    /// @brief      是否停止
    bool isStop() const { return m_isStop; }

    TcpServerConf::ptr getConf() const { return m_conf; }

    void setConf(TcpServerConf::ptr conf){
        m_conf = conf;
    }

    void setConf(const TcpServerConf& v);

private:

    /// @brief        处理新连接的socket类
    virtual void handleClient(Socket::ptr client);


    /// @brief        启动接受socket连接
    virtual void startAccept(Socket::ptr sock);

protected:

    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    uint64_t m_readTimeOut;
    std::string m_name;
    bool m_isStop;


    std::string m_type = "tcp";
    bool m_ssl = false;
    TcpServerConf::ptr m_conf;
};



}