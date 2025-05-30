#include "Thread.h"
#include "logger.h"
#include "util.h"

namespace cxk{

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

void Thread::SetName(const std::string& name){
    if(t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name;

}


Thread::Thread(std::function<void()> cb, const std::string& name):m_cb(cb), m_name(name){
    if(name.empty()){
        m_name = "UNKNOW";
    }
    //CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "thread creator" << std::endl;
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt){
        CXK_LOG_ERROR(g_logger) << "pthread_create error rt = " << rt << std::endl;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}

void* Thread::run(void* arg){
    //CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "Debug" << std::endl;
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = cxk::getThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();

    cb();
    return 0;
}



Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
    }

}


void Thread::join(){
    if(m_thread)    {
        int rt = pthread_join(m_thread, nullptr);
        if(rt){
            CXK_LOG_ERROR(g_logger) << "pthread_join error rt = " << rt <<" name = "<< m_name << std::endl;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }

}

Thread* Thread::GetThis(){
    return t_thread;
}


const std::string& Thread::GetName(){
    return t_thread_name;
}





}
