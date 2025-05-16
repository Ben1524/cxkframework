#pragma once
#include <memory>
namespace cxk{


namespace {

template<class T, class X, int N>
T& GetInstanceX(){
    static T v;
    return v;
}


template<class T, class X, int N>
std::shared_ptr<T> GetInstancePtr(){
    static std::shared_ptr<T> v(new T);
    return v;
}
}




/// @brief      单例模式封装类
/// @tparam T   类型
/// @tparam X   为了创造多个实例对应的Tag
/// @tparam N   为了一个Tag创造多个实例索引
template <typename T, class X = void, int N = 0>
class Singleton
{
public:
    /// @brief 获取单例实例
    /// @return 单例裸指针
    static T *GetInstance()
    {
        // return &GetInstanceX<T, X, N>();
        static T v;
        return &v;
    }

private:
};

/// @brief 单例智能指针封装类
/// @tparam T 类型
/// @tparam X 为了创造多个实例对应的Tag
/// @tparam N 同一个Tag创造多个实例索引
template <typename T, class X = void, int N = 0>
class SingletonPtr
{
public:
    /// @brief 获取单例
    /// @return 单例的智能指针
    static std::shared_ptr<T> GetInstance()
    {
        // return GetInstancePtr<T, X, N>();
        static std::shared_ptr<T> v(new T);
        return v;
    }
};
}