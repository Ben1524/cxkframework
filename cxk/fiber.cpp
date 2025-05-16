#include "fiber.h"
#include <atomic>
#include "config.h"
#include "logger.h"
#include "macro.h"
#include "scheduler.h"

namespace cxk{

static Logger::ptr g_logger = CXK_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id (0);
static std::atomic<uint64_t> s_fiber_count (0);
static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stacksize = Config::Lookup<uint32_t>("fiber.stack_size", 1024*1024, "fiber statck size");


class MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size){
        return free(vp);
    }
};


using StackAllocator = MallocStackAllocator;


Fiber::Fiber(){     //主协程的构造
    // CXK_LOG_DEBUG(g_logger) << "new main fiber";
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)){
        CXK_ASSERT2(false, "getcontext failed");
    }

    ++s_fiber_count;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller):m_id(++s_fiber_id), m_cb(cb){
    // CXK_LOG_DEBUG(g_logger) << "new fiber " << m_id;
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stacksize->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        CXK_ASSERT2(false, "getcontext failed");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller){
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
}


//设置当前协程
void Fiber::SetThis(Fiber* f){
    t_fiber = f;
}


Fiber::~Fiber(){
    --s_fiber_count;
    // CXK_LOG_DEBUG(g_logger) << m_id <<' ' << m_state;
    if(m_stack) {
        CXK_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

        StackAllocator::Dealloc(m_stack, m_stacksize);
    }else {     //主协程
        CXK_ASSERT(!m_cb);
        CXK_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this){
            SetThis(nullptr);
        }
    }  
    // CXK_LOG_DEBUG(g_logger) << "~feber " << m_id;
}

void Fiber::reset(std::function<void()> cb){
    CXK_ASSERT(m_stack);
    CXK_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

    m_cb = cb;
    if(getcontext(&m_ctx)){
        CXK_ASSERT2(false, "getcontext failed");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}


void Fiber::call(){
    SetThis(this);
    m_state = EXEC;
    // CXK_ASSERT(GetThis() == t_threadFiber);
    // CXK_LOG_DEBUG(g_logger) << getId();
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)){
        CXK_ASSERT2(false, "swapcontext failed");
    }
}


void Fiber::swapIn(){
    SetThis(this);
    CXK_ASSERT(m_state != EXEC);
    m_state = EXEC;

    if(swapcontext(&(Scheduler::GetMainFiber())->m_ctx, &m_ctx)){
        CXK_ASSERT2(false, "swapcontext failed");
    }
}


void Fiber::back(){
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)){
        CXK_ASSERT2(false, "swapcontext failed");
    }
}



void Fiber::swapOut(){
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)){
        CXK_ASSERT2(false, "swapcontext failed");
    }
}


Fiber::ptr Fiber::GetThis(){
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    CXK_ASSERT(t_fiber == main_fiber.get());

    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}


    //协程切换到后台，并设置为Ready状态
void Fiber::YieldToReady(){
    Fiber::ptr cur = Fiber::GetThis();
    cur->m_state = READY;
    cur->swapOut();

}
    
    //协程切换到后台，并设置为Hold状态
void Fiber::YieldToHold(){
    Fiber::ptr cur = Fiber::GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFiber(){
    return s_fiber_count;
}


void Fiber::MainFunc(){
    Fiber::ptr cur = Fiber::GetThis();
    CXK_ASSERT(cur);
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(std::exception& e){
        cur->m_state = EXCEPT;
        CXK_LOG_ERROR(g_logger) << "Fiber exception: " << e.what() << "fiber id=" << cur->getId() 
        << std::endl << cxk::BackTraceToString();
    }catch(...){
        cur->m_state = EXCEPT;
        CXK_LOG_ERROR(g_logger) << "Fiber exception: " << "fiber id=" << cur->getId() 
         << std::endl << cxk::BackTraceToString();
    }  

    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->swapOut();
    // CXK_LOG_DEBUG(g_logger) << "Fiber::MainFunc return";
    CXK_ASSERT2(false, "Fiber::MainFunc should not return");
}


void Fiber::CallerMainFunc(){
    Fiber::ptr cur = Fiber::GetThis();
    CXK_ASSERT(cur);
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(std::exception& e){
        cur->m_state = EXCEPT;
        CXK_LOG_ERROR(g_logger) << "Fiber exception: " << e.what() << "fiber id=" << cur->getId() 
        << std::endl << cxk::BackTraceToString();
    }catch(...){
        cur->m_state = EXCEPT;
        CXK_LOG_ERROR(g_logger) << "Fiber exception: " << "fiber id=" << cur->getId() 
         << std::endl << cxk::BackTraceToString();
    }  

    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->back();
    // raw_ptr->swapOut();
    // CXK_LOG_DEBUG(g_logger) << "Fiber::MainFunc return";
    CXK_ASSERT2(false, "Fiber::MainFunc should not return");
}



uint64_t Fiber::GetFiberId(){
    if(t_fiber){
        return t_fiber->getId();
    }
    return 0;
}


}