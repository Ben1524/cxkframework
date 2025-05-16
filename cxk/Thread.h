#pragma once

#include <thread>
#include <pthread.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <sys/syscall.h>
#include <atomic>
#include <sys/types.h>
#include <semaphore.h>
#include <functional>
#include <stdint.h>
#include "noncopyable.h"
#include "mutex.h"

namespace cxk{


/// @brief 线程类
class Thread{
public:
    using ptr = std::shared_ptr<Thread>;
    
    /// @brief      构造函数
    /// @param cb   线程执行函数
    /// @param name 线程名称
    Thread(std::function<void()> cb, const std::string& name);
    
    
    /// @brief 析构函数
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }


    /// @brief 等待线程执行完成
    void join();

    /// @brief 获取当前的线程指针
    static Thread* GetThis();


    /// @brief 获取当前的线程名称
    static const std::string& GetName();

    /// @brief      设置现在的线程名称
    /// @param name 线程名称
    static void SetName(const std::string& name);

private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    /// @brief 线程执行函数
    static void* run(void* arg);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;             
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;
};


}