#pragma once
#include "Thread.h"
//#include "iomanager.h"
#include <memory>
#include <vector>
#include <set>

namespace cxk{

class TimerManager;


/// @brief 定时器
class Timer : public std::enable_shared_from_this<Timer>{
friend class TimerManager;
public:
    using ptr = std::shared_ptr<Timer>;

    /// @brief  取消定时器
    bool cancel();

    /// @brief  刷新设置定时器的执行时间
    bool refresh();

    /// @brief          重置定时器时间
    /// @param ms       定时器执行的间隔时间
    /// @param from_now 是否从当前时间开始计算
    bool reset(uint64_t ms, bool from_now);
private:

    /// @brief              构造函数
    /// @param ms           定时器执行间隔时间
    /// @param cb           回调函数
    /// @param recurring    是否循环
    /// @param manager      定时器管理
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
    
    /// @brief      构造函数
    /// @param next 执行的时间戳（ms）
    Timer(uint64_t next);

private:
    bool m_recurring = false;   //是否循环
    uint64_t m_ms = 0;          //执行周期
    uint64_t m_next = 0;        //精确的执行时间

    std::function<void()> m_cb;  //回调函数
    TimerManager* m_manager = nullptr;

private:
    /// @brief 定时器比较仿函数
    struct Comparetor{
        bool operator()(const Timer::ptr& a, const Timer::ptr& b) const;
    };
};
 

/// @brief 定时器管理类
class TimerManager{
friend class Timer;
public:
    using RWMutexType = cxk::RWMutex;
    TimerManager();

    virtual ~TimerManager();

    /// @brief              添加定时器
    /// @param ms           定时器执行时间间隔
    /// @param cb           定时器回调函数
    /// @param recurring    是否循环定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /// @brief              添加条件定时器
    /// @param ms           定时器执行时间间隔
    /// @param cb           定时器回调函数
    /// @param weak_cond    条件
    /// @param recurring    是否循环
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, 
                bool recurring = false);

    /// @brief  到最近一个定时器执行的时间间隔
    uint64_t getNextTimer();       

    /// @brief      获取需要执行的定时器的回调函数列表
    /// @param cbs  回调函数数组
    void listExpiredCb(std::vector<std::function<void()>>& cbs);

    /// @brief  是否有定时器
    bool hasTimer();
protected:
    /// @brief  当有新的定时器插入到定时器的首部，执行该函数
    virtual void onTimerInsertedAtFront() = 0;
    
    /// @brief      讲定时器添加到管理器中
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);


private:
    /// @brief  检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);

private:
    bool m_tickled = false;
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparetor> m_timers;
    uint64_t m_previousTime = 0;
};


}
