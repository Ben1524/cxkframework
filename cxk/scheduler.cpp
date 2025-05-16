#include "scheduler.h"
#include "logger.h"
#include "macro.h"
#include "hook.h"

namespace cxk{
static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;           //协程调度的主协程

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name): m_name(name){
    //CXK_LOG_DEBUG(g_logger) << "Scheduler() threads = " << threads;
    CXK_ASSERT(threads > 0);

    if(use_caller){
        cxk::Fiber::GetThis();  // 初始化主协程
        --threads;

        CXK_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        cxk::Thread::SetName(m_name);

        t_fiber = m_rootFiber.get();
        m_rootThread = cxk::getThreadId();
        m_threadIds.push_back(m_rootThread);
    }else {
        m_rootThread = -1;
    }
    //CXK_LOG_DEBUG(g_logger) << "Scheduler() m_threadCount = " << threads;
    m_threadCount = threads;
}

void Scheduler::setThis(){
    t_scheduler = this;
} 


void Scheduler::switchTo(int thread){
    CXK_ASSERT(Scheduler::GetThis() != nullptr);
    if(Scheduler::GetThis() == this){
        if(thread == -1 || thread == cxk::getThreadId()){
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idelThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

//协程调度的核心函数
void Scheduler::run(){
    // CXK_LOG_INFO(g_logger) << "Scheduler::run() start";
    set_hook_enable(true);
    setThis();
    if(cxk::getThreadId() != m_rootThread){
        t_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idel, this)));    //空闲协程
    Fiber::ptr cb_fiber;                                                    //用于执行的回调协程      

    FiberAndThread ft;
    while(true){    //从消息队列中取出一个需要执行的协程
    // CXK_LOG_DEBUG(g_logger) << "Scheduler::run() m_fibers.size = " << m_fibers.size();
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();     
            while(it != m_fibers.end()){
                if(it->thread != -1 && it->thread != cxk::getThreadId()){   //不在当前线程,不处理
                    ++it;
                    tickle_me = true;   //需要唤醒其他线程来处理
                    continue;
                }

                CXK_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC){
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);            
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }

        //已经取出来一个需要执行的消息

        if(tickle_me){  //唤醒一个其他线程  
            tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)){    //如果是协程

            ft.fiber->swapIn(); //执行协程

            --m_activeThreadCount;
            if(ft.fiber->getState() == Fiber::READY){   //再次把协程加入队列
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT){
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if(ft.cb){                                       //回调
            if(cb_fiber){
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            // CXK_LOG_INFO(g_logger) << "Scheduler::run() cb" << cb_fiber->getId() << " state = " << cb_fiber->getState();
            ft.reset();
            cb_fiber->swapIn(); 
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY){
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::TERM || cb_fiber->getState() == Fiber::EXCEPT){
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {        //空闲协程
            if(is_active){
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM){
                CXK_LOG_INFO(g_logger) << "idle fiber term";
                // continue;
                break;
            }

            ++m_idelThreadCount;
            idle_fiber->swapIn();
            --m_idelThreadCount;
            if(idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT){
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}


void Scheduler::idel(){
    CXK_LOG_INFO(g_logger) << " idel";
    while(!stopping()){
        cxk::Fiber::YieldToHold();
    }
} 

void Scheduler::tickle(){
    CXK_LOG_INFO(g_logger) << " tickle";
}


bool Scheduler::stopping(){
    MutexType::Lock lock(m_mutex);
    return m_stopping && m_autoStop && m_fibers.empty() && m_activeThreadCount == 0;
}



Scheduler::~Scheduler(){    
    CXK_ASSERT(m_stopping);

    if(GetThis() == this){
        t_scheduler = nullptr;
    }

}


Scheduler* Scheduler::GetThis(){
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber(){
    return t_fiber;
}


void Scheduler::start(){
    MutexType::Lock lock(m_mutex);
    if(!m_stopping){
        return ;
    }
    m_stopping = false;
    CXK_ASSERT(m_threads.empty());
    //CXK_LOG_DEBUG(g_logger) << "start() m_threadCount: " << m_threadCount;
    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i){
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }

    lock.unlock();

    // if(m_rootFiber){
    //     //m_rootFiber->swapIn();
    //     m_rootFiber->call();
    //     CXK_LOG_INFO(g_logger) << "call out";
    // }
}

    
void Scheduler::stop(){
    m_autoStop = true;
    if(m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)){
        CXK_LOG_INFO(g_logger) << this <<" stoped";
        m_stopping = true;

        if(stopping()){     //让子类有清理任务的机会
            return ;
        }
    }

    //bool exit_on_this_fiber = false;
    if(m_rootThread != -1){
        CXK_ASSERT(GetThis() == this);
    }else {
        CXK_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i){
        tickle();
    }

    if(m_rootFiber){
        tickle();
    }

    //如果协程调度使用了当前线程
    if(m_rootFiber){
        // while(!stopping()){
        //     if(m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::EXCEPT){
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
        //         CXK_LOG_INFO(g_logger) << "root fiber is term  ,reset";
        //         t_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }

        if(!stopping()){
            m_rootFiber->call();
        }
    }


    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs){
        i->join();
    }


    // if(stopping()){
    //     return;
    // }
    
}



SchedulerSwitcher::SchedulerSwitcher(Scheduler* target){
    m_caller = Scheduler::GetThis();
    if(target){
        target->switchTo();
    }
}


SchedulerSwitcher::~SchedulerSwitcher(){
    if(m_caller){
        m_caller->switchTo();
    }
}


}