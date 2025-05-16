#include "my_module.h"
#include "cxk/config.h"
#include "cxk/logger.h"


namespace name_space{


static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

MyModule::MyModule(): cxk::Module("project_name", "1.0", ""){

}

bool MyModule::onLoad() {
    CXK_LOG_INFO(g_logger) << "onLoad";
    return true;
}


bool MyModule::onUnload(){
    CXK_LOG_INFO(g_logger) << "onUnload";
    return true;
}


bool MyModule::onServerReady(){
    CXK_LOG_INFO(g_logger) << "onServerReady";
    return true;
}


bool MyModule::onServerUp(){
    CXK_LOG_INFO(g_logger) << "onServerUp";
    return true;
}


}


extern "C" {

cxk::Module* CreateModule(){
    cxk::Module* module = new name_space::MyModule;
    CXK_LOG_INFO(name_space::g_logger) << "CreateModule " << module;
    return module;
}

void DestoryModule(cxk::Module* module){
    CXK_LOG_INFO(name_space::g_logger) << "DestoryModule " << module;
    delete module;
}

}