#include <iostream>
#include <fstream>
#include "cxk/env.h"


struct A{
    A(){
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary); 
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());
        std::cout << content << std::endl;
    }
};


A a;


int main(int argc, char** argv){
    cxk::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    cxk::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    cxk::EnvMgr::GetInstance()->addHelp("p", "print help");
    
    if(!cxk::EnvMgr::GetInstance()->init(argc, argv)){
        cxk::EnvMgr::GetInstance()->printHelp();
        return 0;
    }


    std::cout << "exe=" << cxk::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "pid=" << cxk::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path: " << cxk::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test: " << cxk::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    cxk::EnvMgr::GetInstance()->setEnv("TEST", "123");

    if(cxk::EnvMgr::GetInstance()->has("p")){
        cxk::EnvMgr::GetInstance()->printHelp();
    }

    return 0;
}