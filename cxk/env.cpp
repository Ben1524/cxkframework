#include "env.h"
#include <cstring>
#include "cxk/logger.h"
#include <unistd.h>
#include <iostream>
#include <iomanip>

namespace cxk{

static Logger::ptr g_logger = CXK_LOG_NAME("system");

bool Env::init(int argc, char** argv){

    char link[1024] = {0};
    char path[1024] = {0};
    sprintf(link, "/proc/%d/exe", getpid());    
    if(readlink(link, path, sizeof(path)));

    m_exe = path;
    
    auto pos = m_exe.find_last_of("/");
    m_cwd = m_exe.substr(0, pos) + "/";

    m_program = argv[0];
    // -config /path/to/config -file xx
    const char* now_key = nullptr;
    for(int i = 1; i < argc; ++i){
        if(argv[i][0] == '-'){
            if(strlen(argv[i]) > 1){
                if(now_key){
                    add(now_key, "");
                }
                now_key = argv[i] + 1;
            } else {
                CXK_LOG_ERROR(g_logger) << "invalid key: " << argv[i];
                return false;
            }
        } else {
            if(now_key){
                add(now_key, argv[i]);
                now_key = nullptr;
            } else {
                CXK_LOG_ERROR(g_logger) << "invalid value: " << argv[i];
                return false;
            }
        }
    }
    if(now_key){
        add(now_key, "");
    }
    return true;
}

void Env::add(const std::string& key, const std::string& val){
    RWMutexType::WriteLock lock(m_mutex);
    m_args[key] = val;
}


bool Env::has(const std::string& key){
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end();
}


bool Env::del(const std::string& key){
    RWMutexType::WriteLock lock(m_mutex);
    m_args.erase(key); 
    return true;
}


std::string Env::get(const std::string& key, const std::string& default_val){
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end() ? it->second : default_val;
}


void Env::addHelp(const std::string& key, const std::string& desc){
    removeHelp(key);
    RWMutexType::WriteLock lock(m_mutex);
    m_helps.push_back(std::make_pair(key, desc));
}



void Env::removeHelp(const std::string& key){
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_helps.begin(); it != m_helps.end();){
        if(it->first == key){
            it = m_helps.erase(it);
        } else {
            ++it;
        }
    }
}


void Env::printHelp(){
    RWMutexType::ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_program << " [options]" << std::endl;
    for(auto& i : m_helps){
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}



bool Env::setEnv(const std::string& key, const std::string& val){
    return setenv(key.c_str(), val.c_str(), 1);
}
    
    

std::string Env::getEnv(const std::string& key, const std::string& val){
    const char* v = getenv(key.c_str());
    if(v == nullptr){
        return val;
    }
    return v;
}



std::string Env::getAbsolutePath(const std::string& path) const{
    if(path.empty()) return "/";
    if(path[0] == '/') return path;

    return m_cwd + path;
}


std::string Env::getConfigPath(){
    return getAbsolutePath(get("c", "conf"));
}



}