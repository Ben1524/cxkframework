#pragma once
#include <memory>
#include "fiber.h"
#include "Thread.h"
#include <list>
#include "logger.h"
#include <vector>

namespace cxk{

/// @brief 协程调度器
/// @details 封装的是N-M协程调度器，内部有一个线程池，支持协程在线程池里面切换
class Scheduler{
public:
    using ptr = std::shared_ptr<Scheduler>;
    using MutexType = cxk::Mutex;

    /// @brief              构造函数
    /// @param threads      线程数量
    /// @param use_caller   是否使用当前调用线程
    /// @param name         协程调度器名称
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    /// @brief 析构函数
    virtual ~Scheduler();

    /// @brief 返回调度器名称
    const std::string& getName() const {return m_name;}

    /// @brief  获取当前协程调度器
    static Scheduler* GetThis();

    /// @brief  获取当前协程调度器的调度协程
    static Fiber* GetMainFiber();

    /// @brief  启动协程调度器
    void start();

    /// @brief  停止协程调度器
    void stop();
 

    /// @brief                  调度协程
    /// @param fc               协程或者函数               
    /// @param thread           协程执行的线程id，-1标识任意线程
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_tickle = false;
        {
            
            Mutex::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if(need_tickle){
            tickle();
        }
    }


    /// @brief          批量调度协程              
    /// @param begin    协程数组的开始
    /// @param end      协程数组的结束
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end){
        bool need_tickle = false;
        {
            Mutex::Lock lock(m_mutex);
            while(begin != end){
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle){
            tickle();
        }
    }


    /**
     * @brief 将当前协程切换到当前的调度器并在指定的线程执行
     */
    void switchTo(int thread = -1);
    std::ostream& dump(std::ostream& os);


protected:
    /// @brief  协程调度函数
    void run();                
    
    /// @brief  通知协程调度器有任务了
    virtual void tickle();      
    
    /// @brief  返回协程是否可以停止
    virtual bool stopping();

    /// @brief  协程无任务时执行idle协程
    virtual void idel();

    /// @brief  设置当前的协程调度器
    void setThis();

    /// @brief  是否有空闲线程
    bool hasIdelThreads() {return m_idelThreadCount > 0;}   //判断是否有空闲线程

private:

    /// @brief  协程调度启动(无锁)
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:

    /// @brief 协程/函数/线程组
    struct FiberAndThread{
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread; //线程id

        /// @brief          构造函数
        /// @param fiber    协程
        /// @param thr      线程id
        FiberAndThread(Fiber::ptr fiber, int thr): fiber(fiber), thread(thr){}


        /// @brief      构造函数    
        /// @param f    协程指针
        /// @param thr  线程id
        /// @post       *f = nullptr
        FiberAndThread(Fiber::ptr* f, int thr): thread(thr){
            fiber.swap(*f);
        }


        /// @brief      构造函数
        /// @param cb   协程执行函数
        /// @param thr  线程id
        FiberAndThread(std::function<void()> cb, int thr): cb(cb), thread(thr){}

        /// @brief      构造函数
        /// @param f    协程执行函数指针
        /// @param thr  线程id
        /// @post       *f = nullptr
        FiberAndThread(std::function<void()>* f, int thr):thread(thr){
            cb.swap(*f);
        }

        /// @brief 无参构造函数
        FiberAndThread() : thread(-1){

        }

        /// @brief 重置数据
        void reset(){
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;     // 线程池
    std::list<FiberAndThread> m_fibers;     // 计划要执行的协程队列
    std::string m_name;                     // 协程调度器名称
    Fiber::ptr m_rootFiber;                 // use_calller时有效，调度协程

protected:
    // 协程下的线程id数组
    std::vector<int> m_threadIds;

    // 线程数量
    size_t m_threadCount = 0;

    // 工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};

    // 空闲线程数量
    std::atomic<size_t> m_idelThreadCount = {0};

    // 是否自动停止
    bool m_autoStop = false;

    // 是否正在停止
    bool m_stopping = true;

    // 主线程id(use_caller)
    int m_rootThread = 0;
};


class SchedulerSwitcher : public Noncopyable{
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();
private:
    Scheduler* m_caller;
};


}
