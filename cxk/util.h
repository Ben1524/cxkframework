#pragma once
#include <pthread.h>
#include <cstdint>
#include <vector>   
#include <string>
#include <fstream>
#include <ctime>
#include "util/hash_util.h"
#include <boost/lexical_cast.hpp>
#include "util/crypto_util.h"

namespace cxk{


/// @brief  获取当前线程的ID
pid_t getThreadId();

/// @brief  获取当前协程的ID
uint32_t getFiberId();

/// @brief          获取当前的调用栈
/// @param bt       保存调用栈
/// @param size     最多返回层数
/// @param skip     跳过的栈顶的层数
void BackTrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

/// @brief          获取当前栈信息的字符串
/// @param size     栈的最大层数
/// @param skip     跳过栈的层数
/// @param prefix   栈信息前输出的内容
std::string BackTraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

/// @brief  获取当前时间的毫秒
uint64_t GetCurrentMS();

/// @brief  获取当前时间的微秒
uint64_t GetCurrentUS();


std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");


class FSUtil{
public: 
    /**
     * @brief           列出目录下所有文件
     * 
     * @param files     保存文件名
     * @param path      目录
     * @param subfix    文件后缀
     */
    static void ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix);


    /**
     * @brief           创建目录
     * 
     * @param dirname   目录
     */
    static bool Mkdir(const std::string& dirname);


    /**
     * @brief           判断pid文件代表的进程是否正在运行
     * 
     * @param pidfile   pid文件
     */
    static bool IsRunningPidfile(const std::string& pidfile);

    /**
     * @brief           删除目录下的所有文件
     * 
     * @param path      目录
     */
    static bool Rm(const std::string& path);


    /**
     * @brief           移动文件
     * 
     * @param from      原先文件或者目录的路径
     * @param to        目标位置
     * @return true 
     * @return false 
     */
    static bool Mv(const std::string& from, const std::string& to);


    /**
     * @brief           获取文件的绝对路径
     * 
     * @param path      文件或者目录的路径
     * @param rpath     保存的绝对路径
     */
    static bool Realpath(const std::string& path, std::string& rpath);


    /**
     * @brief           创建符号链接
     * 
     * @param frm       文件或者目录的路径
     * @param to        目标位置
     */
    static bool Symlink(const std::string& frm, const std::string& to);


    /**
     * @brief           删除文件
     * 
     * @param filename  文件名
     * @param exist     文件是否存在
     */
    static bool Unlink(const std::string& filename, bool exist = false);

    /**
     * @brief           获取文件路径的目录部分
     * 
     * @param filename  文件名或者目录的路径
     */
    static std::string Dirname(const std::string& filename);


    /**
     * @brief           获取文件路径的文件名部分
     * 
     */
    static std::string Basename(const std::string& filename);


    /**
     * @brief           用输入的方式打开文件
     * 
     * @param ifs       文件流
     * @param filename  文件名
     * @param mode      打开模式
     */
    static bool OpenForRead(std::ifstream& ifs, const std::string& filename, std::ios_base::openmode mode);

    /**
     * @brief           用输出的方式打开文件
     * 
     * @param ofs       文件流
     * @param filename  文件名
     * @param mode      打开模式
     */
    static bool OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode);
};


template<class V, class Map ,class K>
V GetParamValue(const Map& m, const K& k, const V& def = V()){
    auto it = m.find(k);
    if(it == m.end()){
        return def;
    }
    try{
        return boost::lexical_cast<V>(it->second);
    } catch(...){

    }
    return def;
}


template<class V, class Map, class K>
bool checkGetParamValue(const Map& m, const K& k, V& v){
    auto it = m.find(k);
    if(it == m.end()){
        return false;
    }
    try{
        v = boost::lexical_cast<V>(it->second);
        return true;
    }catch(...){
    }
    return false;
}


template<class T>
void nop(T*) {}

class StringUtil{
public:
    static std::string format(const char* fmt, ...);
    static std::string formatv(const char* fmt, va_list ap);
};



class TypeUtil{
public:
    static int8_t ToChar(const std::string& str);
    static int64_t Atoi(const std::string& str);
    static double Atof(const std::string& str);
    static int8_t ToChar(const char* str);
    static int64_t Atoi(const char* str);
    static double Atof(const char* str);
};



class Atomic{
public:
    template<class T, class S>
    static T addFetch(volatile T& t, S v = 1){
        return __sync_add_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T subFetch(volatile T& t, S v = 1){
        return __sync_sub_and_fetch(&t, (T)v);
    }


    template<class T, class S>
    static T orFetch(volatile T& t, S v) {
        return __sync_or_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T andFetch(volatile T& t, S v) {
        return __sync_and_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T xorFetch(volatile T& t, S v) {
        return __sync_xor_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T nandFetch(volatile T& t, S v) {
        return __sync_nand_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T fetchAdd(volatile T& t, S v = 1) {
        return __sync_fetch_and_add(&t, (T)v);
    }

    template<class T, class S>
    static T fetchSub(volatile T& t, S v = 1) {
        return __sync_fetch_and_sub(&t, (T)v);
    }

    template<class T, class S>
    static T fetchOr(volatile T& t, S v) {
        return __sync_fetch_and_or(&t, (T)v);
    }

    template<class T, class S>
    static T fetchAnd(volatile T& t, S v) {
        return __sync_fetch_and_and(&t, (T)v);
    }

    template<class T, class S>
    static T fetchXor(volatile T& t, S v) {
        return __sync_fetch_and_xor(&t, (T)v);
    }

    template<class T, class S>
    static T fetchNand(volatile T& t, S v) {
        return __sync_fetch_and_nand(&t, (T)v);
    }

    template<class T, class S>
    static T compareAndSwap(volatile T& t, S old_val, S new_val) {
        return __sync_val_compare_and_swap(&t, (T)old_val, (T)new_val);
    }

    /**
     * @brief 比较并交换布尔值
     *
     * 将给定引用 t 的当前值与 old_val 进行比较。如果相等，则将 t 的值更新为 new_val，并返回 true；
     * 如果不相等，则 t 的值保持不变，并返回 false。
     *
     * @param t 需要比较的引用变量
     * @param old_val 期望的旧值
     * @param new_val 要设置的新值
     * @return 如果成功更新 t 的值，则返回 true；否则返回 false
     */
    template<class T, class S>
    static bool compareAndSwapBool(volatile T& t, S old_val, S new_val) {
        return __sync_bool_compare_and_swap(&t, (T)old_val, (T)new_val);
    } 
};


}