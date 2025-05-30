#include "module.h"
#include "config.h"
#include "env.h"
#include "library.h"
#include "util.h"
#include "logger.h"

namespace cxk{

static Logger::ptr g_logger = CXK_LOG_NAME("system");

static cxk::ConfigVar<std::string>::ptr g_module_path = 
    Config::Lookup("module.path", std::string("module"), "module path");


Module::Module(const std::string& name, const std::string& version,
         const std::string& filename, uint32_t type)
    : m_name(name),
    m_version(version),
    m_filename(filename),
    m_id(name + "/" + version),
    m_type(type){

}




void Module::onBeforeArgsParse(int argc, char** argv){

}

RockModule::RockModule(const std::string& name, const std::string& version, const std::string& filename) : Module(name, version, filename, ROCK) {

}



void Module::onAfterArgsParse(int argc, char** argv){

}


bool Module::handleRequest(cxk::Message::ptr req, cxk::Message::ptr rsp, cxk::Stream::ptr stream){
    CXK_LOG_DEBUG(g_logger) << "handleRequest req = " << req->toString() << " rsp = " << rsp->toString() << " stream = " << stream;
    return true;
}


bool Module::handleNotify(cxk::Message::ptr notify, cxk::Stream::ptr stream){
    CXK_LOG_DEBUG(g_logger) << "handleNotify notify = " << notify->toString() << " stream = " << stream;
    return true;
}



bool Module::onLoad(){
    return true;
}



bool Module::onUnload(){
    return true;
}


bool Module::onConnect(cxk::Stream::ptr stream){
    return true;
}



bool Module::onDisconnect(cxk::Stream::ptr stream){
    return true;
}


bool Module::onServerReady(){
    return true;
}


bool Module::onServerUp(){
    return true;
}



std::string Module::statusString(){
    return "";
}


bool RockModule::handleRequest(cxk::Message::ptr req, cxk::Message::ptr rsp, cxk::Stream::ptr stream){
    auto rock_req = std::dynamic_pointer_cast<cxk::RockRequest>(req);
    auto rock_rsp = std::dynamic_pointer_cast<cxk::RockResponse>(rsp);
    auto rock_stream = std::dynamic_pointer_cast<cxk::RockStream>(stream);
    return handleRockRequest(rock_req, rock_rsp, rock_stream);
}

bool RockModule::handleNotify(cxk::Message::ptr notify, cxk::Stream::ptr stream){
    auto rock_notify = std::dynamic_pointer_cast<cxk::RockNotify>(notify);
    auto rock_stream = std::dynamic_pointer_cast<cxk::RockStream>(stream);
    return handleRockNotify(rock_notify, rock_stream);
}


ModuleManager::ModuleManager(){

}

void ModuleManager::add(Module::ptr m){
    del(m->getId());
    RWMutexType::WriteLock lock(m_mutex);
    m_modules[m->getId()] = m;
    m_type2Modules[m->getType()][m->getId()] = m;
}



void ModuleManager::del(const std::string& name){
    Module::ptr module;
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_modules.find(name);
    if(it == m_modules.end()){
        return;
    }

    module = it->second;
    m_modules.erase(it);

    m_type2Modules[module->getType()].erase(module->getId());
    if(m_type2Modules[module->getType()].empty()){
        m_type2Modules.erase(module->getType());
    }

    lock.unlock();
    module->onUnload();
}


void ModuleManager::delAll(){
    RWMutexType::ReadLock lock(m_mutex);
    auto tmp = m_modules;
    lock.unlock();

    for(auto& i : tmp){
        del(i.first);
    }
}


void ModuleManager::init(){
    auto path = EnvMgr::GetInstance()->getAbsolutePath(g_module_path->getValue());

    CXK_LOG_DEBUG(g_logger) << "Module path: " << path;
    std::vector<std::string> files;
    cxk::FSUtil::ListAllFile(files, path, ".so");

    std::sort(files.begin(), files.end());
    for(auto& i : files){
        // CXK_LOG_DEBUG(g_logger) << "Load module: " << i;
        initModule(i);
    }
}


Module::ptr ModuleManager::get(const std::string& name){
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_modules.find(name);
    return it == m_modules.end() ? nullptr : it->second;
}


void ModuleManager::listByType(uint32_t type, std::vector<Module::ptr>& ms){
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_type2Modules.find(type);
    if(it == m_type2Modules.end()){
        return;
    }

    for(auto& i : it->second){
        ms.push_back(i.second);
    }
}


void ModuleManager::foreach(uint32_t type, std::function<void(Module::ptr)> cb){
    std::vector<Module::ptr> ms;
    listByType(type, ms);
    for(auto& i : ms){
        cb(i);
    }
}



void ModuleManager::onConnect(Stream::ptr stream){
    std::vector<Module::ptr> ms;
    listAll(ms);

    for(auto& m : ms){
        m->onConnect(stream);
    }
}


void ModuleManager::onDisconnect(Stream::ptr stream){
    std::vector<Module::ptr> ms;
    listAll(ms);

    for(auto& m : ms){
        m->onDisconnect(stream);
    }
}



void ModuleManager::listAll(std::vector<Module::ptr>& ms){
    RWMutexType::ReadLock lock(m_mutex);
    for(auto& i : m_modules){
        ms.push_back(i.second);
    }
}


void ModuleManager::initModule(const std::string& path){
    Module::ptr m = Library::GetModule(path);
    if(m){
        add(m);
    }
}



}