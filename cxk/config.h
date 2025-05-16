#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <map>
#include <cstring>
#include "logger.h"
#include <set>
#include <vector>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include "Thread.h"

namespace cxk{

/// @brief 配置变量的基类
class ConfigVarBase{
public:
    using ptr = std::shared_ptr<ConfigVarBase>;

    /// @brief 构造函数
    /// @param name 配置参数名称
    /// @param description 配置参数描述
    ConfigVarBase(const std::string &name, const std::string &description = "") : m_destription(description),
                                                                                    m_name(name){
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_destription; }

    /// @brief 转为字符串
    virtual std::string toString() = 0;
    
    /// @brief 从字符串初始化值
    virtual bool fromString(const std::string &value) = 0;
    
    /// @brief 返回配置参数值的类型名称
    virtual std::string getTypeName() const = 0;

protected:
    std::string m_destription;
    std::string m_name;
};


    
/// @brief 类型转换模板类偏特化
/// @tparam F 源类型
/// @tparam T 目标类型
template <class F, class T>
class LexicalCast
{
public:

    /// @brief 类型转换
    /// @param v 源类型值
    /// @return 返回v转换后的目标类型
    /// @exception 当类型不可转换时抛出异常
    T operator()(const F &v)
    {
        return boost::lexical_cast<T>(v);
    }

private:
};


/// @brief 类型转换模板类的偏特化，从YAML String 转换为 std::vector<T>
/// @tparam T 目标类型
template <class T>
class LexicalCast<std::string, std::vector<T>>
{
public:
    std::vector<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};


/// @brief 类型转换模板类的偏特化， std::vector<T> 转换为 YAML String
template <class T>
class LexicalCast<std::vector<T>, std::string>
{
public:
    std::string operator()(const std::vector<T> &v)
    {
        YAML::Node node(YAML::NodeType::Sequence);

        for (auto &i : v)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/// @brief 类型转换模板类的偏特化， YAML 转换为 list<T>
template <class T>
class LexicalCast<std::string, std::list<T>>
{
public:
    std::list<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/// @brief 类型转换模板类片特化(std::list<T> 转换成 YAML String)
template <class T>
class LexicalCast<std::list<T>, std::string>
{
public:
    std::string operator()(const std::list<T> &v)
    {
        YAML::Node node(YAML::NodeType::Sequence);

        for (auto &i : v)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/// @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
template <class T>
class LexicalCast<std::string, std::set<T>>
{
public:
    std::set<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};



/// @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
template <class T>
class LexicalCast<std::set<T>, std::string>
{
public:
    std::string operator()(const std::set<T> &v)
    {
        YAML::Node node(YAML::NodeType::Sequence);

        for (auto &i : v)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/// @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
template <class T>
class LexicalCast<std::string, std::unordered_set<T>>
{
public:
    std::unordered_set<T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};


/// @brief  类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
template <class T>
class LexicalCast<std::unordered_set<T>, std::string>
{
public:
    std::string operator()(const std::unordered_set<T> &v)
    {
        YAML::Node node(YAML::NodeType::Sequence);

        for (auto &i : v)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};



/// @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
template <class T>
class LexicalCast<std::string, std::map<std::string, T>>
{
public:
    std::map<std::string, T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};


/// @brief  类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
template <class T>
class LexicalCast<std::map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::map<std::string, T> &v)
    {
        YAML::Node node(YAML::NodeType::Map);

        for (auto &i : v){
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};



/// @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string, T>)
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>>
{
public:
    std::unordered_map<std::string, T> operator()(const std::string &v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};



/// @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML String)
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::unordered_map<std::string, T> &v)
    {
        YAML::Node node(YAML::NodeType::Map);

        for (auto &i : v)
        {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/// @brief 配置参数模板子类，保存对应类型的参数值
/// @tparam T           参数的具体类型
/// @tparam FromStr     从std::string转换为T的仿函数
/// @tparam ToStr       为YAML格式的字符串
template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
{
public:
    using ptr = std::shared_ptr<ConfigVar<T>>;
    using on_change_cb = std::function<void(const T &old_value, const T &new_value)>;
    using RWMutexType = cxk::RWMutex;


    /// @brief 通过参数名，参数值，描述构造ConfigVar
    /// @param name         参数有效字符为[0-9a-z_.]
    /// @param value        参数的默认值
    /// @param description  参数的描述
    ConfigVar(const std::string &name, const T &value, const std::string &description = "") : ConfigVarBase(name, description), m_value(value)
    {
    }


    /// @brief 将参数值转换为YAML String
    /// @exception 当转换失败抛出异常
    virtual std::string toString() override
    {
        try
        {
            // return boost::lexical_cast<std::string>(m_value);
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_value);
        }
        catch (std::exception &e)
        {
            CXK_LOG_ERROR(CXK_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                                            << "convert: " << typeid(m_value).name() << "to string";
        }
        return "";
    }


    /// @brief 从YAML String转换为参数的值
    /// @exception 转换失败抛出异常
    virtual bool fromString(const std::string &value) override
    {
        try
        {
            // m_value = boost::lexical_cast<T>(value);
            setValue(FromStr()(value));
            return true; // 不确定
        }
        catch (std::exception &e)
        {
            CXK_LOG_ERROR(CXK_LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
                                            << "convert: string to " << typeid(m_value).name();
        }
        return false;
    }

    /// @brief 获取当前参数的值
    T getValue(){
        RWMutexType::ReadLock lock(m_mutex);
        return m_value; 
    }


    /// @brief 生物质当前参数的值
    /// @details 如果参数的值有发生变化，则通知对应的注册回调函数
    void setValue(const T &value){
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_value == value)
                return;
            for (auto &i : m_cbs){
                i.second(m_value, value);
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        m_value = value;
    }


    /// @brief 获取参数值的类型名称typeinfo
    std::string getTypeName() const { return typeid(T).name(); }


    /// @brief 添加变化回调函数
    /// @return 返回该回调函数的唯一id，用于删除回调 
    uint64_t addListener(on_change_cb cb){
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }


    /// @brief 删除回调函数
    /// @param key  回调函数的唯一id
    void delListener(uint64_t key){
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }


    /// @brief 获取回调函数
    /// @param key  回调函数的唯一id
    /// @return     如果存在返回对应的回调函数，否则返回nullptr
    on_change_cb getListener(uint64_t key)
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    /// @brief 清理所有的回调函数
    void clearListener(){
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    T m_value;
    RWMutexType m_mutex;
    // 变更回调函数组, uint64_t key要求唯一, 一般用hash值
    std::map<uint64_t, on_change_cb> m_cbs;
};



/// @brief ConfigVar的管理类
/// @details 提供便利的方法来创建/访问ConfigVar
class Config
{
public:
    using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;
    using RWMutexType = cxk::RWMutex;


    /// @brief 获取/创建对应参数名的配置参数
    /// @param name             配置参数名称 
    /// @param default_value    参数默认值
    /// @param destription      参数描述
    /// @return                 返回对应的配置参数，如果存在但是类型不匹配返回nullptr
    /// @details                获取参数名为name的配置参数，如果不存在则创建并用default_value赋值
    /// @exception              如果参数名包含非法字符，抛出异常
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string destription = ""){
        RWMutexType::WriteLock lock(GetMutex());
        auto it = GetData().find(name);
        if (it != GetData().end()){
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp){
                CXK_LOG_INFO(CXK_LOG_ROOT()) << "Config::Lookup: " << name << " already exist";
                return tmp;
            }
            else{
                CXK_LOG_ERROR(CXK_LOG_ROOT()) << "Config::Lookup: " << name << " already exist, but type not " << typeid(T).name()
                                                << "real type = " << it->second->getTypeName() << it->second->toString();
                return nullptr;
            }
        }

        if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_.") != std::string::npos){
            CXK_LOG_ERROR(CXK_LOG_ROOT()) << "Config::Lookup: invalid name: " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, destription));
        GetData()[name] = v;

        return v;
    }

    /// @brief 找到配置参数
    /// @param name 配置参数名称 
    /// @return     返回配置参数名为name的配置参数
    template <class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name){
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetData().find(name);
        if (it == GetData().end()){
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /// @brief 查找配置参数， 返回配置参数的基类
    /// @param name 配置参数名称
    static ConfigVarBase::ptr LookupBase(const std::string &name);

    /**
     * @brief       从配置目录加载配置文件
     * 
     */
    static void LoadFromConfDir(const std::string& path, bool force = false);


    /// @brief 使用YAML::Node初始化配置模块
    static void LoadFromYaml(const YAML::Node &root);

    /// @brief 遍历配置模块里面的所有配置项
    /// @param ch   配置项回调函数
    static void Visit(std::function<void(ConfigVarBase::ptr)> ch);

private:
    /// @brief 返回所有的配置项
    static ConfigVarMap& GetData() {
        static ConfigVarMap m_datas;
        return m_datas;
    }

    /// @brief 配置项的RWMutex
    static RWMutexType& GetMutex() {
        static RWMutexType m_mutex;
        return m_mutex;
    }
};

}
