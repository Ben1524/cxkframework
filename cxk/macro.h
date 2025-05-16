#pragma once
#include <string.h>
#include <assert.h>
#include "util.h"
#include "logger.h"


#if defined __GNUC__ || defined __llvm__
#    define CXK_LICKLY(x)       __builtin_expect(!!(x), 1)
#    define CXK_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#    define CXK_LIKELY(x)       (x)
#    define CXK_UNLIKELY(x)     (x)
#endif

#define CXK_ASSERT(x)                                     \
    if(CXK_UNLIKELY(!(x))){                                             \
        CXK_LOG_ERROR(CXK_LOG_ROOT()) << "ASSERTION: " #x \
            <<"\nbacktrace:\n"                            \
            << cxk::BackTraceToString(100, 2, "    ");    \
        assert(x);                                        \
    }


#define CXK_ASSERT2(x, w) \
        if(CXK_UNLIKELY(!(x))){                                         \
        CXK_LOG_ERROR(CXK_LOG_ROOT()) << "ASSERTION: " #x \
            <<"\n" << w                                   \
            <<"\nbacktrace:\n"                            \
            << cxk::BackTraceToString(100, 2, "    ");    \
        assert(x);                                        \
    }



