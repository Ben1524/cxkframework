#pragma once


#include <list>
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
#include "fiber.h"



namespace cxk{



/// @brief 信号量
class Semaphore : Noncopyable{
public:
    /// @brief 构造函数
    /// @param count    信号量的值
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    /// @brief 获取信号量
    void wait();

    /// @brief 释放信号量
    void notify();

private:
    sem_t m_semaphore;
};


/// @brief 局部锁的模板实现
template<class T>
struct ScopedLockImpl{
public:
    /// @brief 构造函数
    /// @param mutex   锁
    ScopedLockImpl(T& mutex) : m_mutex(mutex){
        m_mutex.lock();
        m_locked = true;
    }

    /// @brief 析构函数，自动释放锁
    ~ScopedLockImpl(){
        unlock();
    }


    /// @brief 加锁
    void lock() {
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }

    /// @brief 解锁
    void unlock() {
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


private:
    T& m_mutex;
    bool m_locked;  //是否已经上锁
};



/// @brief 局部读锁模板实现
template<class T>
struct ReadScopedLockImpl{
public:
    /// @brief 构造函数
    /// @param mutex    读写锁
    ReadScopedLockImpl(T& mutex) : m_mutex(mutex){
        m_mutex.rdlock();
        m_locked = true;
    }

    /// @brief 析构函数，自动释放锁
    ~ReadScopedLockImpl(){
        unlock();
    }

    /// @brief 上读锁
    void lock() {
        if(!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    /// @brief 释放锁
    void unlock() {
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


private:
    T& m_mutex;
    bool m_locked;

};



/// @brief 局部写锁模板实现
template<class T>
struct WriteScopedLockImpl{
public:
    /// @brief 构造函数
    /// @param mutex    读写锁
    WriteScopedLockImpl(T& mutex) : m_mutex(mutex){
        m_mutex.wrlock();
        m_locked = true;
    }

    /// @brief 析构函数
    ~WriteScopedLockImpl(){
        // m_locked->unlock();
        unlock();
    }

    /// @brief 上写锁
    void lock() {
        if(!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    /// @brief 解锁
    void unlock() {
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


private:
    T& m_mutex;
    bool m_locked;
};



/// @brief 互斥量
class Mutex : Noncopyable{
public:
    using Lock = ScopedLockImpl<Mutex>;

    /// @brief 构造函数
    Mutex(){
        pthread_mutex_init(&m_mutex, NULL);
    }


    /// @brief 析构函数
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }

    /// @brief 加锁
    void lock(){
        pthread_mutex_lock(&m_mutex);
    }

    /// @brief 解锁
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};



/// @brief 空锁，用于调试
class NullMutex : Noncopyable{
public:
    using Lock = ScopedLockImpl<NullMutex>;

    NullMutex(){
    }

    ~NullMutex(){
    }
    void lock(){
    }

    void unlock(){
    }

private:

};


/// @brief 空读写锁，用于调试
class NullRWMutex : Noncopyable{
public:
    using ReadLock = ReadScopedLockImpl<NullRWMutex>;
    using WriteLock = WriteScopedLockImpl<NullRWMutex>;
    NullRWMutex(){
    }

    ~NullRWMutex(){
    }

    void rdlock(){
    }

    void wrlock(){
    }

    void unlock(){
    }
};


/// @brief 读写互斥量
class RWMutex  : Noncopyable{
public:
    using ReadLock = ReadScopedLockImpl<RWMutex>;
    using WriteLock = WriteScopedLockImpl<RWMutex>;

    /// @brief 构造函数
    RWMutex() {
        pthread_rwlock_init(&m_lock, NULL);
    }

    /// @brief 析构函数
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    /// @brief 上读锁
    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    /// @brief 上写锁
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    /// @brief 解锁
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};

/// @brief 自旋锁
class Spinlock : Noncopyable{
public:

    using Lock = ScopedLockImpl<Spinlock>;

    /// @brief 构造函数
    Spinlock(){
        pthread_spin_init(&m_mutex, 0);
    }


    /// @brief 析构函数
    ~Spinlock(){
        pthread_spin_destroy(&m_mutex);
    }

    /// @brief 上锁
    void lock(){
        pthread_spin_lock(&m_mutex);
    }


    /// @brief 解锁
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }

    
private:
    pthread_spinlock_t m_mutex;
};


/// @brief 原子锁
class CASLock : Noncopyable{
public:
    using Lock = ScopedLockImpl<CASLock>;
    
    /// @brief 构造函数
    CASLock(){
        m_mutex.clear();
    }

    /// @brief 析构函数
    ~CASLock(){
    }

    /// @brief 加锁
    void lock(){
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    /// @brief 解锁
    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;    
};



class Scheduler;


/**
 * @brief 协程信号量
 */
class FiberSemaphore : Noncopyable{
public:
    using MutexType = Spinlock;

    FiberSemaphore(size_t initial_concurrency = 0);
    ~FiberSemaphore();

    /**
     * @brief 尝试等待,尝试减少一个信号量的计数
     * @return true 成功
     * @return false 失败, 当前没有足够的并发度计数
     */
    bool tryWait();


    /**
     * @brief 等待,减少一个信号量的计数, 如果当前没有足够的并发度计数, 则阻塞等待
     */
    void wait();
    void notify();

    size_t getConcurrency() const { return m_concurrency; }
    void reset() { m_concurrency = 0; }

private:
    MutexType m_mutex;
    std::list<std::pair<Scheduler*, Fiber::ptr> > m_waiters;
    size_t m_concurrency;       // 当前可用的并发度
};


}