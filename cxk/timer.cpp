#include "timer.h"
#include "util.h"
#include "logger.h"

namespace cxk{

bool Timer::Comparetor::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const{
    if(!lhs && !rhs) return false;
    
    if(!lhs) return true;
    if(!rhs) return false;
    if(lhs->m_next < rhs->m_next) return true;
    if(lhs->m_next > rhs->m_next) return false;
    return lhs.get() < rhs.get();
}


Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
    :m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager){
    m_next = cxk::GetCurrentMS() + m_ms;
}


Timer::Timer(uint64_t next) : m_next(next){

}


bool Timer::cancel(){
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb){
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh(){      //重置定时器
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) return false;

    auto it = m_manager->m_timers.find(shared_from_this());     
    if(it == m_manager->m_timers.end()) return false;

    m_manager->m_timers.erase(it);
    m_next = cxk::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}


bool Timer::reset(uint64_t ms, bool from_now){
    if(ms == m_ms && !from_now){
        return true;    
    }


    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) return false;

    auto it = m_manager->m_timers.find(shared_from_this());     
    if(it == m_manager->m_timers.end()) return false;

    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now){
        start = cxk::GetCurrentMS();
    }else {
        start = m_next - m_ms; 
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}


TimerManager::TimerManager(){
    m_previousTime = cxk::GetCurrentMS();
} 


TimerManager::~TimerManager(){

}


bool TimerManager::hasTimer(){
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}


Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring){
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);

    addTimer(timer, lock);
    //CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "add timer finish";
    return timer;
}

static void onTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb){
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp){
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, 
                bool recurring){
    return addTimer(ms, std::bind(onTimer, weak_cond, cb), recurring);
}


uint64_t TimerManager::getNextTimer(){
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()){
        return ~0ull;
    }
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = cxk::GetCurrentMS();
    if(now_ms >= next->m_next){
        return 0;
    } else {
        return next->m_next - now_ms;   //还需等待的时间
    }
}


//这个函数的主要作用是从一个定时器集合 m_timers 中找出所有已经到期的定时器，
//并将它们的回调函数 m_cb 添加到 cbs 参数向量中
void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs){
    uint64_t now_ms = cxk::GetCurrentMS();
    std::vector<Timer::ptr> expired;    //已经到期的定时器

    {
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timers.empty()){       //没有定时器要执行
            return;
        }
    }

    //CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "try write lock";
    RWMutexType::WriteLock lock(m_mutex);
    //CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "write lock success";
    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)){
        //CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "no timer to expire";
        return;
    }
    //CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "timer to expire";
    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover? m_timers.end() : m_timers.lower_bound(now_timer);     //第一个将要到期的定时器


    while(it != m_timers.end() && (*it)->m_next == now_ms){
        ++it;
    }

    

    expired.insert(expired.end(), m_timers.begin(), it);    //将所有到期的定时器添加到 expired 
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());    //为 cbs 预留空间

    for(auto& timer : expired){
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring){     //如果是重复定时器
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}


void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock){
    auto it = m_timers.insert(val).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front){
        m_tickled = true;
    }

    lock.unlock();

    if(at_front){   //插入了一个即将进行的定时器
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms){
    bool rollover = false;
    if(now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)){
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}

}