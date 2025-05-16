#include "cxk/config.h"
#include <iostream>
#include <thread>
#include "cxk/logger.h"
#include "cxk/util.h"
#include <yaml-cpp/yaml.h>
#include "cxk/env.h"



void test_loadconf(){
    cxk::Config::LoadFromConfDir("conf");
}

int main(int argc, char *argv[])
{
    cxk::EnvMgr::GetInstance()->init(argc, argv);
    test_loadconf();
    std::cout << "=================" << std::endl;
    test_loadconf();

    return 0;
    cxk::Config::Visit([](cxk::ConfigVarBase::ptr var){
        CXK_LOG_INFO(CXK_LOG_ROOT()) <<var->getName() <<" " << var->getDescription() << " " <<var->getTypeName() << " " << var->toString();
    });
    return 0;
}
