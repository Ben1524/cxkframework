#include "hook.h"
#include <dlfcn.h>
#include "fiber.h"
#include "iomanager.h"
#include "logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <functional>
#include <sys/socket.h>
#include "fd_manager.h"
#include "macro.h"
#include "config.h"

namespace cxk{
static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

static thread_local bool t_hook_enable = false;
static cxk::ConfigVar<int>::ptr g_tcp_connect_timeout = 
    cxk::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)


void hook_init(){
    static bool is_inited = false;
    if(is_inited) return ;

#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}


static uint64_t s_connect_timeout = -1;
struct _HookIniter{
    _HookIniter(){
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_val, const int& new_val){
            CXK_LOG_INFO(g_logger) << "tcp connect timeout changed" ;
            s_connect_timeout = new_val;
        });
    }
};

static _HookIniter s_hook_initer;


bool is_hook_enable(){
    return t_hook_enable;
}



void set_hook_enable(bool flag){
    t_hook_enable = flag;
}



struct timer_info{
    int cancelled = 0;
};




template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event, 
        int timeout_so, Args&&... args){
    if(!cxk::t_hook_enable){
        return fun(fd, std::forward<Args>(args)...);    //完美转发先前的函数
    }


    cxk::FdCtx::ptr ctx = cxk::FdMgr::GetInstance()->get(fd);
    if(!ctx){
        return fun(fd, std::forward<Args>(args)...);
    }
    if(ctx->isClose()){
        errno = EBADF;  //错误的文件描述符
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNoblock()){
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeOut(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);
    // CXK_LOG_DEBUG(g_logger) << "hook " << hook_fun_name;

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while(n == -1 && errno == EINTR){
        n = fun(fd, std::forward<Args>(args)...);
    }
    //CXK_LOG_DEBUG(g_logger) << "n == " << n << " errno == " << errno; 
    if(n == -1 && errno == EAGAIN){

        cxk::IOManager* iom = cxk::IOManager::GetThis();
        cxk::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if(to != (uint64_t)-1){     //有超时时间
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
                auto t = winfo.lock();
                if(!t || t->cancelled){
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (cxk::IOManager::Event)(event));
            }, winfo);
        }
        

        int rt = iom->addEvent(fd, (cxk::IOManager::Event)(event));
        if(CXK_UNLIKELY(rt)){
            CXK_LOG_ERROR(g_logger) << "addEvent( " << fd << " ) error";

            if(timer){
                timer->cancel();
            }
            return -1;
        } else {
            cxk::Fiber::YieldToHold();
            if(timer){
                timer->cancel();
            }
            if(tinfo->cancelled){
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }
    return n;
}


extern "C"{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX


unsigned int sleep(unsigned int seconds){
    if(!cxk::t_hook_enable){
        return sleep_f(seconds);    //返回原函数
    }

    cxk::Fiber::ptr fiber = cxk::Fiber::GetThis();
    cxk::IOManager* iom = cxk::IOManager::GetThis();

    // iom->addTimer(seconds*1000, std::bind(&cxk::IOManager::schedule<Fiber::ptr>, iom, fiber));
    // iom->addTimer(seconds * 1000, [iom, fiber](){
    //     iom->schedule(fiber);
    // });
    iom->addTimer(seconds * 1000, std::bind((void(cxk::Scheduler::*)(cxk::Fiber::ptr, int thread))
            &cxk::IOManager::schedule, iom, fiber, -1));


    cxk::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec){
    if(!cxk::t_hook_enable){
        return usleep_f(usec);    //返回原函数
    }

    cxk::Fiber::ptr fiber = cxk::Fiber::GetThis();
    cxk::IOManager* iom = cxk::IOManager::GetThis();

    // iom->addTimer(usec / 1000, std::bind(&cxk::IOManager::schedule, iom, fiber));
    // iom->addTimer(usec / 1000, [iom, fiber](){
    //     iom->schedule(fiber);
    // });

    iom->addTimer(usec / 1000, std::bind((void(cxk::Scheduler::*)(cxk::Fiber::ptr, int thread))&cxk::IOManager::schedule, 
                iom, fiber, -1));
    cxk::Fiber::YieldToHold();
    return 0;
}


int nanosleep(const struct timespec* req, struct timespec* rem){
    if(!cxk::t_hook_enable){
        return nanosleep_f(req, rem);
    }
    int timeout_ms = req->tv_sec * 1000 + rem->tv_nsec / 1000 / 1000;
    
    cxk::Fiber::ptr fiber = cxk::Fiber::GetThis();
    cxk::IOManager* iom = cxk::IOManager::GetThis();

    // iom->addTimer(usec / 1000, std::bind(&cxk::IOManager::schedule, iom, fiber));
    // iom->addTimer(timeout_ms, [iom, fiber](){
    //     iom->schedule(fiber);
    // });

    iom->addTimer(timeout_ms, std::bind((void(cxk::Scheduler::*)(cxk::Fiber::ptr, int thread))
        &cxk::IOManager::schedule, iom, fiber, -1));
    cxk::Fiber::YieldToHold();
    return 0;
}


int socket(int domain, int type, int protocol){
    if(!cxk::t_hook_enable){
        return socket_f(domain, type, protocol);
    }

    int fd = socket_f(domain, type, protocol);
    if(fd == -1){
        return fd;
    }
    cxk::FdMgr::GetInstance()->get(fd, true);
    return fd;
}


/*
该函数的主要目的是尝试在给定的超时时间内连接到一个套接字。
若设置了超时时间，会创建一个定时器在超时后取消写事件

*/

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms){
    if(!cxk::t_hook_enable){
        return connect_f(fd, addr, addrlen);
    }




    CXK_LOG_DEBUG(g_logger) << "connect_with_timeout, fd: " << fd;
    cxk::FdCtx::ptr ctx = cxk::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()){
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()){
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->getUserNoblock()){
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);
    if(n == 0){
        return 0;
    } else if(n != -1 || errno != EINPROGRESS){
        return n;
    }
    CXK_LOG_DEBUG(g_logger) << "connecting, fd: " << fd;

    cxk::IOManager* iom = cxk::IOManager::GetThis();
    cxk::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t) -1){
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom](){
            std::shared_ptr<timer_info> t = winfo.lock();
            if(!t || t->cancelled){
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, cxk::IOManager::WRITE);
        }, winfo);
    }

    int rt = iom->addEvent(fd, cxk::IOManager::WRITE);      // 添加写事件判断是否成功连接
    if(rt == 0){
        cxk::Fiber::YieldToHold();
        if(timer){
            timer->cancel();
        }
        if(tinfo->cancelled){
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if(timer){
            timer->cancel();
        }
        CXK_LOG_ERROR(g_logger) << "addEvent failed, fd: " << fd;
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)){   //判断是否连接成功
        CXK_LOG_ERROR(g_logger) << "getsockopt failed, fd: " << fd;
        return -1;
    }
    if(!error){
        return 0;
    }else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    return connect_with_timeout(sockfd, addr, addrlen, cxk::s_connect_timeout);
}


int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    int fd = do_io(sockfd, accept_f, "accept", cxk::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0){
        cxk::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}


ssize_t read(int fd, void *buf, size_t count){
    return do_io(fd, read_f, "read", cxk::IOManager::READ, SO_RCVTIMEO, buf, count);
}


ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, readv_f, "readv", cxk::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}


ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    return do_io(sockfd, recv_f, "recv", cxk::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}


ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen){
    return do_io(sockfd, recvfrom_f, "recvfrom", cxk::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}


ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    return do_io(sockfd, recvmsg_f, "recvmsg", cxk::IOManager::READ, SO_RCVTIMEO, msg, flags);
}


ssize_t write(int fd, const void *buf, size_t count){
    return do_io(fd, write_f, "write", cxk::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}


ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, writev_f, "writev", cxk::IOManager::WRITE, SO_SNDTIMEO, iov,iovcnt);
}



ssize_t send(int sockfd, const void *buf, size_t len, int flags){
    return do_io(sockfd, send_f, "send", cxk::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}


ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen){
    return do_io(sockfd, sendto_f, "sendto", cxk::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}


ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    return do_io(sockfd, sendmsg_f, "sendmsg", cxk::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}


int close(int fd){
    if(!cxk::t_hook_enable){
        return close_f(fd);
    }

    cxk::FdCtx::ptr ctx = cxk::FdMgr::GetInstance()->get(fd);
    if(ctx){
        auto iom = cxk::IOManager::GetThis();
        if(iom){
            iom->cancelAll(fd);
        }
        cxk::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}


int fcntl(int fd, int cmd, ... /* arg */ ){
    va_list va;
    va_start(va, cmd);
    switch (cmd){
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                cxk::FdCtx::ptr ctx = cxk::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()){
                    return fcntl_f(fd, cmd, arg);
                }

                ctx->setUserNoblock(arg & O_NONBLOCK);
                if(ctx->getSysNoblock()){
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;

        case F_GETFL:
            {
                va_end(va);
                int arg  = fcntl_f(fd, cmd);
                cxk::FdCtx::ptr ctx = cxk::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()){
                    return arg;
                }
                if(ctx->getUserNoblock()){
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {  
                struct flock *arg = va_arg(va, struct flock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_ex *arg = va_arg(va, struct f_owner_ex *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
    }
}


int ioctl(int fd, unsigned long request, ...){
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request){
        bool user_nonblock = !!*(int*)arg;
        cxk::FdCtx::ptr ctx = cxk::FdMgr::GetInstance()->get(fd);
        if(!ctx || ctx->isClose() || !ctx->isSocket()){
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNoblock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}


int getsockopt(int sockfd, int level, int optname,
                      void *optval, socklen_t *optlen){
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}


int setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen){
    if(!cxk::t_hook_enable){
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET){
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO){
            cxk::FdCtx::ptr ctx = cxk::FdMgr::GetInstance()->get(sockfd);
            if(ctx){
                const timeval* tv = (const timeval*)optval;
                ctx->setTimeOut(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}   // extern "C"

}   // namespace cxk