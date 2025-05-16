#include "cxk/cxk.h"
#include <assert.h>


cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void test_assert(){
    // CXK_LOG_INFO(g_logger) << cxk::BackTraceToString(10, 2, "    ");
    CXK_ASSERT2(false, "hello world");
}

int main(int argc, char **argv){
    test_assert();

    return 0;
}

