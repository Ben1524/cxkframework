#include "cxk/cxk.h"
#include <vector>

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void run_in_fiber(){
    CXK_LOG_INFO(g_logger) << "run in fiber Begin";
    cxk::Fiber::YieldToHold();
    CXK_LOG_INFO(g_logger) << "run in fiber End";
    cxk::Fiber::YieldToHold();
}

void test_fiber(){
    CXK_LOG_INFO(g_logger) << "main begin -1";
    {
        cxk::Fiber::GetThis();
        CXK_LOG_INFO(g_logger) << "main begin";
        cxk::Fiber::ptr fiber(new cxk::Fiber(run_in_fiber));

        fiber->swapIn();
        CXK_LOG_INFO(g_logger) << "main after swapin";
        fiber->swapIn();

        CXK_LOG_INFO(g_logger) << "main end";
        fiber->swapIn();
    }
    CXK_LOG_INFO(g_logger) << "main end2";
}

int main(int argc, char** argv){
    cxk::Thread::SetName("main");

    std::vector<cxk::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i){
        thrs.push_back(cxk::Thread::ptr(new cxk::Thread(&test_fiber, "name:" + std::to_string(i))));
    }

    for(auto i : thrs){
        i->join();
    }

    return 0;
}