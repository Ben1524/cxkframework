#include "cxk/cxk.h"

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

int count = 0;
// cxk::RWMutex mutex;
cxk::Mutex mutex;

void fun1(){
    CXK_LOG_INFO(g_logger) << "name: " << cxk::Thread::GetName() << " this.name: " <<cxk::Thread::GetThis()->getName()<<
    " id: " << cxk::getThreadId() << " this.id: " << cxk::Thread::GetThis()->getId() << std::endl;

    for(int i = 0; i < 1000000; ++i){
        cxk::Mutex::Lock lock(mutex);
        count++;
    }
}

void fun2(){
    while(true){
        CXK_LOG_INFO(g_logger) <<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
        //std::cout<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" <<std::endl;
        
        // sleep(1);
    }
}

void fun3(){
    while(true){
        CXK_LOG_INFO(g_logger) <<"--------------------------------------------------------------";
        std::cout<<"----------------------------------------------------------------" <<std::endl;
        
        // sleep(1);
    }
}

int main(int argc, char* argv[]){

    CXK_LOG_INFO(g_logger) << "thread test start" << std::endl;

    // YAML::Node root = YAML::LoadFile("/home/cxk/work/web-serve/bin/conf/log2.yml");
    // cxk::Config::LoadFromYaml(root);

    std::vector<cxk::Thread::ptr> thrs;

    for(int i = 0; i < 2; i++){
        cxk::Thread::ptr thr(new cxk::Thread(&fun2, "function2: " + std::to_string(i * 2)));
        cxk::Thread::ptr thr2(new cxk::Thread(&fun3, "function3: " + std::to_string(i * 2 + 1)));

        thrs.push_back(thr2);
        thrs.push_back(thr);
    }

    for(size_t i = 0; i < thrs.size(); i++){
        thrs[i]->join();
    }

    CXK_LOG_INFO(g_logger) << "count: " << count << std::endl;
    return 0;
}

