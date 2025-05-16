#pragma once 
#include <memory>
#include "Thread.h"
#include "iomanager.h"
#include "Singleton.h"
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

namespace cxk{

/// @brief  文件句柄上下文类
/// @details    管理文件句柄类型（是否socket）是否阻塞，是否关闭，读写超时时间
class FdCtx : public std::enable_shared_from_this<FdCtx>{
public:
    using ptr = std::shared_ptr<FdCtx> ;

    /// @brief      通过文件句柄构造FdCtx
    FdCtx(int fd);

    /// @brief  析构函数
    ~FdCtx();

    /// @brief  初始化
    bool init();

    /// @brief  是否初始化完成
    bool isInit() const {return m_isInit;}

    /// @brief  是否是socket
    bool isSocket() const {return m_isSocket;}

    /// @brief  是否关闭
    bool isClose() const {return m_isClosed;}


    /// @brief      设置用户主动设置非阻塞
    /// @param v    是否阻塞
    void setUserNoblock(bool v) {m_userNoblock = v;}
    
    /// @brief  获取是否用户主动设置的非阻塞
    bool getUserNoblock() const {return m_userNoblock;}

    /// @brief      设置系统非阻塞
    /// @param v    是否阻塞
    void setSysNoblock(bool v) {m_sysNoblock = v;}

    /// @brief  获取系统非阻塞
    bool getSysNoblock() const {return m_sysNoblock;}

    /// @brief          设置超时时间
    /// @param type     类型SO_RECVTIMEO（读超时），SO_SNDTIMEO(写超时)
    /// @param v        时间毫秒
    void setTimeOut(int type, uint64_t v);

    /// @brief      获取超时时间
    uint64_t getTimeOut(int type);

private:
    bool m_isInit: 1;
    bool m_isSocket: 1;
    bool m_sysNoblock: 1;       // 是否hook非阻塞
    bool m_userNoblock: 1;      // 是否用户主动设置非阻塞
    bool m_isClosed: 1;
    int m_fd;
    
    uint64_t m_recvTimeOut;
    uint64_t m_sendTimeOut;
};


/// @brief  文件句柄管理类
class FdManager{
public:
    using RWMutexType = RWMutex;

    /// @brief  无参构造函数
    FdManager();

    /// @brief              获取/创建文件句柄类FdCtx
    /// @param fd           fd文件句柄
    /// @param auto_create  是否自动创建
    /// @return             返回对应文件句柄类FdCtx::ptr
    FdCtx::ptr get(int fd, bool auto_create = false);


    /// @brief      删除文件句柄类
    /// @param fd   文件句柄
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;

};


/// @brief  文件句柄单例
typedef Singleton<FdManager> FdMgr;

}
