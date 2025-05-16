#pragma once
#include <ucontext.h>
#include <memory>
// #include "Thread.h"
#include <functional>
// #include "mutex.h"

namespace cxk{


/// @brief 协程类
class Fiber : public std::enable_shared_from_this<Fiber>{
    friend class Scheduler;
public:
    using ptr = std::shared_ptr<Fiber>;

    /// @brief 协程状态
    enum State{ 
        INIT,       // 初始化状态
        HOLD,       // 暂停状态
        EXEC,       // 执行中状态
        TERM,       // 结束状态
        READY,      // 可执行状态
        EXCEPT      // 异常状态
    };

private:
    /// @brief 构造函数
    /// @attention 每个线程第一个协程的构造
    Fiber();

public:
    /// @brief              构造函数
    /// @param cb           协程执行的函数
    /// @param stacksize    协程栈的大小
    /// @param use_caller   是否在MainFiber上调度
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    /// @brief 重置协程执行函数，并设置其状态
    /// @pre getState() 为INT,TERM,EXCEPT
    /// @post getState() 为INIT
    void reset(std::function<void()> cb);

    /// @brief  将当前携程切换到运行状态
    /// @pre    getState() != EXEC
    /// @post   getState() = EXEC
    void swapIn();

    /// @brief  将协程切换到后台
    void swapOut();

    /// @brief  将当前协程切换到执行状态
    /// @pre    执行的为当前线程的主协程
    void call();    

    /// @brief  将当前协程切换到后台
    /// @pre    执行的该协程
    /// @post   返回到线程的主协程
    void back();

    /// @brief 返回协程ID
    uint64_t getId() const { return m_id; }

    /// @brief 返回协程状态
    State getState() const { return m_state; }

public:
    /// @brief      设置线程的运行协程
    /// @param f    运行协程
    static void SetThis(Fiber* f);

    /// @brief      返回当前所在的协程
    static Fiber::ptr GetThis();

    /// @brief  将当前协程切换到后代，并设置READY状态
    /// @post   getState() = READY
    static void YieldToReady();


    /// @brief  将当前协程切换到后台，并设置为HOLD状态
    /// @post   getState() = HOLD
    static void YieldToHold();

    /// @brief  返回当前协程的总数量
    static uint64_t TotalFiber();

    /// @brief  协程执行函数
    /// @post   执行完成返回到线程主线程
    static void MainFunc();

    /// @brief  协程执行函数
    /// @post   执行完成返回到线程调度协程
    static void CallerMainFunc();

    /// @brief  获取当前协程的ID 
    static uint64_t GetFiberId();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;    
    void* m_stack = nullptr;        // 协程运行栈指针
    std::function<void()> m_cb;     // 协程运行函数
};



}