#pragma once
#include "mutex.h"
#include "Singleton.h"
#include "logger.h"
#include "iomanager.h"



namespace cxk{

class WorkerGroup : Noncopyable, public std::enable_shared_from_this<WorkerGroup>{
public:
    using ptr = std::shared_ptr<WorkerGroup>;
    static WorkerGroup::ptr Create(uint32_t batch_size, cxk::Scheduler* s = cxk::Scheduler::GetThis()){
        return std::make_shared<WorkerGroup>(batch_size, s);
    }

    WorkerGroup(uint32_t batch_size, cxk::Scheduler* s = cxk::Scheduler::GetThis());
    ~WorkerGroup();

    void schedule(std::function<void()> cb, int thread = -1);
    void waitAll();

private:
    void doWork(std::function<void()> cb);

private:
    uint32_t m_batchSize;
    bool m_finish;
    Scheduler* m_scheduler;
    FiberSemaphore m_sem;
};



class WorkerManager{
public:
    WorkerManager();
    void add(Scheduler::ptr s);
    Scheduler::ptr get(const std::string& name);
    IOManager::ptr getAsIOManager(const std::string& name);

    template<class FiberOrCb>
    void schedule(const std::string& name, FiberOrCb fc, int thread = -1){
        auto s = get(name);
        if(s){
            s->schedule(fc, thread);
        } else {
            static cxk::Logger::ptr s_logger = CXK_LOG_NAME("system");
            CXK_LOG_ERROR(s_logger) << "scheduler name = " << name << " not exists";
        }
    }


    template<class Iter>
    void schedule(const std::string& name, Iter begin, Iter end){
        auto s = get(name);
        if(s){
            s->schedule(begin, end);
        } else {
            static cxk::Logger::ptr s_logger = CXK_LOG_NAME("system");
            CXK_LOG_ERROR(s_logger) << "schdule name = " << name << " not exists";
        }
    }

    bool init();
    void stop();

    bool isStoped() const { return m_stop; }
    std::ostream& dump(std::ostream& os);

    uint32_t getCount();

private:
    std::map<std::string, std::vector<Scheduler::ptr>> m_datas;
    bool m_stop;
};

using WorkerMgr = cxk::Singleton<WorkerManager>;


}