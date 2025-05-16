#include "library.h"


#include <dlfcn.h>
#include "config.h"
#include "env.h"
#include "logger.h"


namespace cxk{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

using create_module = Module*(*)();
using destory_module = void(*)(Module*);


class ModuleCloser{
public:
    ModuleCloser(void* handle, destory_module d) : m_handle(handle), m_destory(d){
    }

    void operator()(Module* module){
        std::string name = module->getName();
        std::string version = module->getVersion();
        std::string path = module->getFilename();
        m_destory(module);
        int rt = dlclose(m_handle);
        if(rt){
            CXK_LOG_ERROR(g_logger) << "dlclose(" << path << ") failed. errno: " << rt;
        } else {
            CXK_LOG_INFO(g_logger) << "destory moudle = " << name
            << " version = " << version
            << " path = " << path
            << " handle = " << m_handle
            << " success.";
        }
    }

private:
    void* m_handle;
    destory_module m_destory;
};



Module::ptr Library::GetModule(const std::string& path){
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if(!handle){
        CXK_LOG_ERROR(g_logger) << "dlopen(" << path << ") failed. errno: " << dlerror();
        return nullptr;
    }
    create_module create = (create_module)dlsym(handle, "CreateModule");
    if(!create){
        CXK_LOG_ERROR(g_logger) << "cannot load symbol CreateModule in " << path;
        dlclose(handle);
        return nullptr;
    }

    destory_module destory = (destory_module)dlsym(handle, "DestoryModule");
    if(!destory){
        CXK_LOG_ERROR(g_logger) << "cannot load symbol DestoryModule in " << path;
        dlclose(handle);
        return nullptr;
    }

    Module::ptr module(create(), ModuleCloser(handle, destory));
    module->setFilename(path);
    CXK_LOG_INFO(g_logger) << "load module name = " << module->getName()
    << " version = " << module->getVersion()
    << " path = " << module->getFilename()
    << " success";
    // Config::LoadFromConfDir(cxk::EnvMgr::GetInstance()->getConfigPath(), true);
    return module;
}


}