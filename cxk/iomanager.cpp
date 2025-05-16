#include "iomanager.h"
#include <sys/epoll.h>
#include "macro.h"
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <cstring>


namespace cxk{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

// 通过event来返回一个EventContext
IOManager::FdContext::EventContext& IOManager::FdContext::getcontext(IOManager::Event event){
    switch (event)
    {
    case IOManager::Event::READ:
        return read;
        break;
    case IOManager::Event::WRITE:
         return write;
    default:
        CXK_ASSERT2(false, "getcontext error");
        break;
    }
    throw std::invalid_argument("getcontext error");
}

void IOManager::FdContext::resetContext(EventContext& ctx){
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}


// 触发对应event事件
void IOManager::FdContext::triggerEvent(IOManager::Event event){
    CXK_ASSERT(events & event);
    events = (Event)(events & (~event));
    EventContext& ctx = getcontext(event);

    if(ctx.cb){
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}


/*
    IOManager的构造函数,创建epoll，并监听管道的读事件
*/
IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name){
    m_epfd = epoll_create(5000);
    CXK_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    CXK_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET;   //设置边缘触发
    event.data.fd = m_tickleFds[0];     //设置读管道为事件fd

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);    //设置读管道为非阻塞
    CXK_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);  //将读管道添加到epoll中
    CXK_ASSERT(!rt);

    contextResize(32);

    // CXK_LOG_DEBUG(g_logger) << "IOManager()  start";
    start();
    // CXK_LOG_DEBUG(g_logger) << "IOManager()  end";
}


IOManager::~IOManager(){
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); i++){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}


/*
    给m_fdContexts进行扩容
*/
void IOManager::contextResize(size_t size){
    //CXK_LOG_DEBUG(g_logger) << "into contextResize size = " << size;
     m_fdContexts.resize(size);

     for(size_t i = 0; i < m_fdContexts.size(); i++){
         if(!m_fdContexts[i]){
             m_fdContexts[i] = new FdContext();
             m_fdContexts[i]->fd = i;
         }
     }

    //CXK_LOG_DEBUG(g_logger) << "out contextResize" << size;
}


/*
    通过fd找到对应的FdContext，如果不存在回重新分配内存
    将该事件修改或添加到epoll
*/
int IOManager::addEvent(int fd, Event events, std::function<void()> cb){
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd){
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(CXK_UNLIKELY(fd_ctx->events & events)){
         CXK_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd << " events = " << events;
          CXK_ASSERT(!(fd_ctx->events & events));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;    //判断是修改还是增加
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        CXK_LOG_ERROR(g_logger) << "epoll_ctl error fd = " << fd << " events = " << events;
        return -1;
    }


    ++m_appendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | events);
    FdContext::EventContext& event_ctx = fd_ctx->getcontext(events);
    CXK_ASSERT(event_ctx.scheduler == nullptr);
    CXK_ASSERT(event_ctx.cb == nullptr);
    CXK_ASSERT(event_ctx.fiber == nullptr);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        CXK_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }

    return 0;
}


bool IOManager::delEvent(int fd, Event events){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(CXK_UNLIKELY(!(fd_ctx->events & events))){
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & (~events));
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        CXK_LOG_ERROR(g_logger) << "epoll_ctl error fd = " << fd << " events = " << events;
        return false;
    }

    --m_appendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getcontext(events);
    fd_ctx->resetContext(event_ctx);    //清理掉

    return true;
}


/* 
    从epoll中删掉fd对应的事件，并且执行对应的events事件
*/
bool IOManager::cancelEvent(int fd, Event events){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(CXK_UNLIKELY(!(fd_ctx->events & events))){
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & (~events));
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        CXK_LOG_ERROR(g_logger) << "epoll_ctl error fd = " << fd << " events = " << events;
        return false;
    }
    
    fd_ctx->triggerEvent(events);
    --m_appendingEventCount;
    return true;
}


bool IOManager::cancelAll(int fd){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events)){
        return false;
    }

    int op = EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        CXK_LOG_ERROR(g_logger) << "epoll_ctl error fd = " << fd;
        return false;
    }

    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_appendingEventCount;
    }
    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_appendingEventCount;
    }

    CXK_ASSERT(fd_ctx->events == 0);    
    return true;
}


IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager*> (Scheduler::GetThis());
}


/*
    如果没有空闲线程，则向管道写入一个事件来触发响应
*/
void IOManager::tickle(){
    if(!hasIdelThreads()){   
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    CXK_ASSERT(rt == 1);
}


bool IOManager::stopping(uint64_t& timeout){
    timeout = getNextTimer();
    return timeout == ~0ull && m_appendingEventCount == 0 && Scheduler::stopping();
}


bool IOManager::stopping(){
    uint64_t timeout = 0;
    return stopping(timeout);
}


void IOManager::idel(){
    // epoll_event* events = new epoll_event[64]();
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event> shared_events(events,[](epoll_event* ptr){
        delete[] ptr;
    });     //使用智能指针来管理内存


    while(true){
        uint64_t next_timeout = 0;
        if(CXK_UNLIKELY(stopping(next_timeout))){
            CXK_LOG_INFO(g_logger) << "idel stopping";
            break;
        }

        int rt = 0;
        do{
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull){
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
            if(rt < 0 && errno == EINTR){
            } else {
                break;
            }
        }while(true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);

        if(!cbs.empty()){
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for(int i = 0; i < rt; ++i){
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]){    //有外部消息发给我们
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)){
                events->events |= EPOLLIN | EPOLLOUT;
            }

            int real_events = NONE;
            if(event.events & EPOLLIN){ //读事件
                real_events |= READ;
            }

            if(event.events & EPOLLOUT){ //写事件
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE){
                continue;
            }

            int left_events = (fd_ctx->events & (~real_events));
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2){
                CXK_LOG_ERROR(g_logger) << "epoll_ctl error fd = " << fd_ctx->fd;
                continue;
            }

            if(fd_ctx->events & READ){             //触发读事件
                fd_ctx->triggerEvent(READ);
                --m_appendingEventCount;
            }

            if(fd_ctx->events & WRITE){            //触发写事件
                fd_ctx->triggerEvent(WRITE);
                --m_appendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();     //让出协程执行权
    }
}


void IOManager::onTimerInsertedAtFront(){
    tickle();
}


}