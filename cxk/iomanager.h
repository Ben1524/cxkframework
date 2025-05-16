#pragma once

#include "scheduler.h"
#include "timer.h"

namespace cxk{

/// @brief 基于epoll的io协程调度类
class IOManager : public Scheduler, public TimerManager {
public:
    using ptr = std::shared_ptr<IOManager>;
    using RWMutexType = cxk::RWMutex;   //使用读写锁  

    /// @brief IO事件
    enum Event{
        NONE = 0x0,     // NOEVENT
        READ = 0x1,     // EPOLLIN
        WRITE = 0x4,    // EPOLLOUT
    };
private:
    /// @brief Socket事件上下文类
    struct FdContext{
        using MutexType = cxk::Mutex;

        /// @brief 事件上下文类
        struct EventContext{
            // 事件执行的调度器
            Scheduler* scheduler = nullptr;
            // 事件fiber      
            Fiber::ptr fiber;           
            std::function<void()> cb;   //事件的回调函数
        };


        /// @brief          获取事件上下文类
        /// @param event    事件类型
        /// @return         返回对应的事件上下文
        EventContext& getcontext(Event event);

        /// @brief      重置事件上下文
        /// @param ctx  待重置的上下文
        void resetContext(EventContext& ctx);

        /// @brief          触发事件
        /// @param event    事件类型
        void triggerEvent(Event event);
  

        int fd = 0;             // 事件关联的句柄
        EventContext read;      // 读事件
        EventContext write;     // 写事件
        Event events = NONE;    // 当前的事件
        MutexType mutex;
    };

public:
    /// @brief              构造函数
    /// @param threads      线程数量
    /// @param use_caller   是否将调用线程含进去
    /// @param name         调度器的名称
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    
    /// @brief  析构函数
    ~IOManager();

    /// @brief          添加事件
    /// @param fd       fd socket句柄
    /// @param events   事件类型
    /// @param cb       事件回调函数
    /// @return         添加成功返回0， 失败返回-1
    int addEvent(int fd, Event events, std::function<void()> cb  = nullptr);

    /// @brief          删除事件
    /// @param fd       fd socket句柄
    /// @param events   事件类型
    /// @attention      不会触发事件
    bool delEvent(int fd, Event events);

    /// @brief          取消事件
    /// @param fd       fd socket句柄
    /// @param events   事件类型
    /// @attention      如果事件存在则触发事件
    bool cancelEvent(int fd, Event events);

    /// @brief      取消所有的事件
    /// @param fd   fd socket的句柄 
    bool cancelAll(int fd);               

    /// @brief  获取当前的IOManger 
    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idel() override;

    /// @brief          判断是否可以停止
    /// @param timeout  最近要发出的定时器事件间隔
    /// @return         返回是否可以停止
    bool stopping(uint64_t& timeout);

    /// @brief      重置socket句柄上下文的容器大小
    /// @param size 容量大小
    void contextResize(size_t size);

    void onTimerInsertedAtFront() override;

private:
    int m_epfd = 0;
    int m_tickleFds[2]; //文件句柄

    std::atomic<size_t> m_appendingEventCount = {0};    //等待要执行的事件数量
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};


}