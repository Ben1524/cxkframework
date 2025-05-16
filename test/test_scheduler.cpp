#include "cxk/cxk.h"

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void test_fiber() {
    CXK_LOG_INFO(g_logger) << "test_fiber";

    sleep(2);  

    CXK_LOG_INFO(g_logger) << "test_fiber over";
    static int i = 0;

    if(++i < 5){
        cxk::Scheduler::GetThis()->schedule(&test_fiber);
    }
}


int main(int argc, char *argv[]){
    cxk::Scheduler sc(5, false, "test");

    CXK_LOG_DEBUG(g_logger) << "start====================";
    sc.start();

    CXK_LOG_DEBUG(g_logger) << "schedule============";

    sc.schedule(&test_fiber);

    CXK_LOG_DEBUG(g_logger) << "stop====================";
    sc.stop();

    CXK_LOG_INFO(g_logger) << "over===============";

    return 0;
}