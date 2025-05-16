#pragma once


#include <memory>
#include "bytearray.h"

namespace cxk{


/// @brief  流结构
class Stream{
public:
    using ptr = std::shared_ptr<Stream>;
    
    /// @brief  析构函数
    virtual ~Stream() {}

    /// @brief          读数据
    /// @param buffer   接受数据的内存
    /// @param length   读数据的内存大小
    /// @return         @retval >= 0  读到的数据大小, @retval = 0  被关闭, @retval < 0  出错
    virtual int read(void* buffer, size_t length) = 0;


    /// @brief          读数据
    /// @param ba       接收数据的ByteArray
    /// @param length   读数据的内存大小
    /// @return         @retval >= 0  读到的数据大小, @retval = 0  被关闭, @retval < 0  出错
    virtual int read(ByteArray::ptr ba, size_t length) = 0;


    /// @brief          读固定长度数据
    /// @param buffer   接受数据的内存
    /// @param length   读数据的内存大小
    /// @return         @retval >= 0  读到的数据大小, @retval = 0  被关闭, @retval < 0  出错
    virtual int readFixSize(void* buffer, size_t length);

    /// @brief          读固定长度数据
    /// @param ba       接收数据的ByteArray
    /// @param length   读数据的内存大小
    /// @return         @retval >= 0  读到的数据大小, @retval = 0  被关闭, @retval < 0  出错
    virtual int readFixSize(ByteArray::ptr ba, size_t length);



    /// @brief          写数据
    /// @param buffer   写数据的内存
    /// @param length   写数据的内存大小
    /// @return         @retval >= 0  写的数据大小, @retval < 0  出错
    virtual int write(const void* buffer, size_t length) = 0;

    /// @brief          写数据
    /// @param ba       写数据的内存
    /// @param length   写数据的内存大小
    /// @return         @retval >= 0  写的数据大小, @retval < 0  出错
    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    /// @brief          写固定长度数据
    /// @param buffer   写数据的内存
    /// @param length   写数据的内存大小
    /// @return         @retval >= 0  写的数据大小, @retval < 0  出错
    virtual int writeFixSize(const void* buffer, size_t length);

    /// @brief          写固定长度数据
    /// @param ba       写数据的内存
    /// @param length   写数据的内存大小
    /// @return         @retval >= 0  写的数据大小, @retval < 0  出错
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);


    /// @brief          关闭流
    virtual void close() = 0;
};

}

