#include "cxk/daemon.h"
#include "cxk/iomanager.h"
#include "cxk/logger.h"


static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();
cxk::Timer::ptr timer ;


int server_main(int argc, char** argv){
    CXK_LOG_INFO(g_logger) << cxk::ProcessInfoMgr::GetInstance()->toString();
    cxk::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
        CXK_LOG_INFO(g_logger) << "hello";
        static int count = 0;
        if(++count > 10){
            timer->cancel();
        }
    }, true);
}


int main(int argc, char** argv){
    return cxk::start_daemon(argc, argv, server_main, argc != 1);
}