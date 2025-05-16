#include "application.h"
#include "cxk/config.h"
#include "cxk/env.h"
#include "cxk/logger.h"
#include "cxk/daemon.h"
#include "module.h"
#include "cxk/work.h"
#include <unistd.h>
#include "cxk/http/http_server.h"
#include <unistd.h>
#include "tcp_server.h"
#include "rock/rock_server.h"
#include "http/ws_server.h"


namespace cxk{


static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

static cxk::ConfigVar<std::string>::ptr g_server_work_path =
    cxk::Config::Lookup("server.work_path"
            ,std::string("~/Work/cxkframework/bin")
            , "server work path");

static cxk::ConfigVar<std::string>::ptr g_server_pid_file = cxk::Config::Lookup("server.pid_file", 
    std::string("cxk.pid"), "server pid file");


static cxk::ConfigVar<std::vector<TcpServerConf>>::ptr g_servers_conf = 
    cxk::Config::Lookup("servers", std::vector<TcpServerConf>(), "http servers conf");


Application* Application::s_instance = nullptr;

Application::Application(){
    s_instance = this;
}

bool Application::init(int argc, char** argv){
    m_argc = argc;
    m_argv = argv;
    cxk::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    cxk::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    cxk::EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
    cxk::EnvMgr::GetInstance()->addHelp("p", "print help");

    bool is_print_help = false;

    if(!cxk::EnvMgr::GetInstance()->init(argc, argv)){
        is_print_help = true;
    }

    if(cxk::EnvMgr::GetInstance()->has("p")){
        is_print_help = true;
    }

    std::string conf_path = cxk::EnvMgr::GetInstance()->getConfigPath();
    CXK_LOG_INFO(g_logger) << "load conf path:" << conf_path;
    cxk::Config::LoadFromConfDir(conf_path);

    ModuleMgr::GetInstance()->init();
    std::vector<Module::ptr> modules;
    ModuleMgr::GetInstance()->listAll(modules);

    for(auto i : modules){
        i->onBeforeArgsParse(argc, argv);
    }

    if(is_print_help){
        cxk::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    for(auto i : modules){
        i->onAfterArgsParse(argc, argv);
    }
    modules.clear();

    int run_type = 0;
    if(cxk::EnvMgr::GetInstance()->has("s")){
        run_type = 1;
    }
    if(cxk::EnvMgr::GetInstance()->has("d")){
        run_type = 2;
    }

    if(run_type == 0){
        cxk::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();

    if(cxk::FSUtil::IsRunningPidfile(pidfile)){
        CXK_LOG_ERROR(g_logger) << "server is running" << pidfile;
        return false;
    }

    if(!cxk::FSUtil::Mkdir(g_server_work_path->getValue())){
        CXK_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue() << " errno=" <<
            errno << " errstr=" << strerror(errno) << "]";
            return false;
    }

    // std::ofstream ofs(pidfile);
    // if(!ofs){
    //     CXK_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
    //     return false;
    // }

    // ofs << getpid();

    return true;
}



bool Application::run(){
    bool is_daemon = cxk::EnvMgr::GetInstance()->has("d");
    return start_daemon(m_argc, m_argv, std::bind(&Application::main, this, std::placeholders::_1, std::placeholders::_2), is_daemon);
}



int Application::main(int argc, char** argv){
    std::string conf_path = cxk::EnvMgr::GetInstance()->getConfigPath();

    cxk::Config::LoadFromConfDir(conf_path, true);
    {
        std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
        std::ofstream ofs(pidfile);
        if(!ofs){
            CXK_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
            return false;
        }
    
        ofs << getpid();
    }




    // cxk::IOManager iom(1);
    // iom.schedule(std::bind(&Application::run_fiber, this));
    // iom.stop();

    m_mainIOManager.reset(new cxk::IOManager(1, true, "main"));
    m_mainIOManager->schedule(std::bind(&Application::run_fiber, this));
    m_mainIOManager->addTimer(2000, [](){
    }, true);

    m_mainIOManager->stop();

    return 0;
}


int Application::run_fiber(){
    std::vector<Module::ptr> modules;
    ModuleMgr::GetInstance()->listAll(modules);
    bool has_error = false;
    for(auto &i : modules){
        if(!i->onLoad()){
            CXK_LOG_ERROR(g_logger) << "module load failed" << 
                i->getName() << " " << i->getVersion() << " " << i->getFilename();
            has_error = true;
        }
    }

    if(has_error){
        _exit(0);
    }

    cxk::WorkerMgr::GetInstance()->init();

    auto http_confs = g_servers_conf->getValue();

    for(auto& i : http_confs){
        CXK_LOG_INFO(g_logger) << LexicalCast<TcpServerConf, std::string>()(i) << " " << i.type;
        std::vector<Address::ptr> address;
        for(auto& a : i.address){
            size_t pos = a.find(":");
            if(pos == std::string::npos){
                // CXK_LOG_ERROR(g_logger) << "invalid address: " << a;
                address.push_back(UnixAddress::ptr(new UnixAddress(a)));
                continue;
            }
            int32_t port = atoi(a.substr(pos + 1).c_str());
            auto addr = cxk::IPAddress::Create(a.substr(0, pos).c_str(), port);
            if(addr){
                address.push_back(addr);
                continue;
            }
            std::vector<std::pair<Address::ptr, uint32_t>> result;
            if(cxk::Address::GetInterfaceAddresses(result, a.substr(0, pos))){
                for(auto & x : result){
                    auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                    if(ipaddr){
                        ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                    }
                    address.push_back(ipaddr);
                }
                continue;
            }
            auto aaddr = cxk::Address::LookupAny(a);
            if(aaddr){
                address.push_back(aaddr);
                continue;
            }
            CXK_LOG_ERROR(g_logger) << "invalid address: " << a;
            _exit(0);
        }


        IOManager* accept_worker = cxk::IOManager::GetThis();
        IOManager* process_worker = cxk::IOManager::GetThis();
        if(!i.accept_worker.empty()){
            accept_worker = cxk::WorkerMgr::GetInstance()->getAsIOManager(i.accept_worker).get();
            if(!accept_worker){
                CXK_LOG_ERROR(g_logger) << "invalid accept worker: " << i.accept_worker;
                _exit(0);
            }
        }

        if(!i.process_worker.empty()){
            process_worker = cxk::WorkerMgr::GetInstance()->getAsIOManager(i.process_worker).get();
            if(!process_worker){
                CXK_LOG_ERROR(g_logger) << "invalid process worker: " << i.process_worker;
                _exit(0);
            }
        }

        // cxk::http::HttpServer::ptr server(new cxk::http::HttpServer((i.keepalive), process_worker, accept_worker));
        TcpServer::ptr server;
        if(i.type == "http"){
            server.reset(new cxk::http::HttpServer((i.keepalive), process_worker, accept_worker));
        } else if(i.type == "ws"){
            server.reset(new cxk::http::WSServer(process_worker, accept_worker));
        }else if(i.type == "rock"){
            server.reset(new cxk::RockServer(process_worker, accept_worker));
        }
        else{
            CXK_LOG_ERROR(g_logger) << "invalid server type: " << i.type;
            _exit(0);
        }

        if(!i.name.empty()){
            server->setName(i.name);
        }

        std::vector<Address::ptr> fails;
        if(!server->bind(address, fails, i.ssl)){
            for(auto& a : fails){
                CXK_LOG_ERROR(g_logger) << "bind address failed: " << a->toString();
            }
            _exit(0);
        }

        if(i.ssl){
            if(!server->loadCertificates(i.cert_file, i.key_file)){
                CXK_LOG_ERROR(g_logger) << "load ssl cert failed, cert_file=" << i.cert_file << ", key_file=" << i.key_file;
            }
        }
        server->setConf(i);


        // //===== cxk test ====================
        // server_cb(server);
        // //====================================
        
        server->start();
        // m_httpservers.push_back(server);
        m_servers[i.type].push_back(server);
    }

    for(auto& i : modules){
        i->onServerReady();
    }

    // while(true) {
    //     CXK_LOG_INFO(g_logger) << "hello world";
    //     usleep(1000 * 100);
    // }
    return 0;
}


bool Application::getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs){
    auto it = m_servers.find(type);
    if(it == m_servers.end()){
        return false;
    }
    svrs = it->second;
    return true;
}


}