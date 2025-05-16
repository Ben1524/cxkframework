#pragma once

#include "cxk/stream.h"
#include "cxk/socket.h"
#include "cxk/mutex.h"
#include "cxk/iomanager.h"

namespace cxk{

/// @brief  socket流
class SocketStream : public Stream{
public:
    using ptr = std::shared_ptr<SocketStream>;

    /// @brief          构造函数
    /// @param socket   socket类
    /// @param owner    是否完全控制
    SocketStream(Socket::ptr socket, bool owner = true);    
    
    /// @brief      析构函数
    /// @details    如果拥有socket则析构时关闭socket
    ~SocketStream();


    /// @brief          读数据
    /// @param buffer   接受数据的内存
    /// @param length   读数据的内存大小
    /// @return         @retval >= 0  读到的数据大小, @retval = 0  被关闭, @retval < 0  出错
    virtual int read(void* buffer, size_t length) override;



    /// @brief          读数据
    /// @param ba       接收数据的ByteArray
    /// @param length   读数据的内存大小
    /// @return         @retval >= 0  读到的数据大小, @retval = 0  被关闭, @retval < 0  出错
    virtual int read(ByteArray::ptr ba, size_t length) override;


    /// @brief          写数据
    /// @param buffer   写数据的内存
    /// @param length   写数据的内存大小
    /// @return         @retval >= 0  写的数据大小, @retval < 0  出错
    virtual int write(const void* buffer, size_t length) override;


    /// @brief          写数据
    /// @param ba       写数据的ByteArray
    /// @param length   写数据的内存大小
    /// @return         @retval >= 0  写的数据大小, @retval < 0  出错
    virtual int write(ByteArray::ptr ba, size_t length) override;


    /// @brief          是否连接
    bool isConnected() const;

    /// @brief          关闭
    virtual void close() override;


    /// @brief          获取socket句柄
    Socket::ptr getSocket() const { return m_socket; }

protected:
    Socket::ptr m_socket;
    bool m_owner;
};


}