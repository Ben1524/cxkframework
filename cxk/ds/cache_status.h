#pragma once

#include <sstream>
#include <string>
#include <stdint.h>
#include "cxk/util.h"



namespace cxk{
namespace ds{


class CacheStatus{
public:
    CacheStatus() {}

    int64_t incGet(int64_t v = 1) { return Atomic::addFetch(m_get, v);}
    int64_t incSet(int64_t v = 1) { return Atomic::addFetch(m_set, v);}
    int64_t incDel(int64_t v = 1) { return Atomic::addFetch(m_del, v);}
    int64_t incTimeout(int64_t v = 1) { return Atomic::addFetch(m_timeout, v);}
    int64_t incPrune(int64_t v = 1) { return Atomic::addFetch(m_prune, v);}
    int64_t incHit(int64_t v = 1) { return Atomic::addFetch(m_hit, v);}

    int64_t decGet(int64_t v = 1) { return Atomic::subFetch(m_get, v);}
    int64_t decSet(int64_t v = 1) { return Atomic::subFetch(m_set, v);}
    int64_t decDel(int64_t v = 1) { return Atomic::subFetch(m_del, v);}
    int64_t decTimeout(int64_t v = 1) { return Atomic::subFetch(m_timeout, v);}
    int64_t decPrune(int64_t v = 1) { return Atomic::subFetch(m_prune, v);}
    int64_t decHit(int64_t v = 1) { return Atomic::subFetch(m_hit, v);}

    int64_t getGet() const { return m_get;}
    int64_t getSet() const { return m_set;}
    int64_t getDel() const { return m_del;}
    int64_t getTimeout() const { return m_timeout;}
    int64_t getPrune() const { return m_prune;}
    int64_t getHit() const { return m_hit;}


    double getHitRate() const {
        return m_get ? (m_hit * 1.0 / m_get) : 0;
    }


    void merge(const CacheStatus& o){
        m_get += o.m_get;
        m_set += o.m_set;
        m_del += o.m_del;
        m_timeout += o.m_timeout;
        m_prune += o.m_prune;
        m_hit += o.m_hit;
    }


    std::string toString() const {
        std::stringstream ss;
        ss << "CacheStatus{get=" << m_get 
            << ", set=" << m_set
            << ", del=" << m_del
            << ", timeout=" << m_timeout
            << ", prune=" << m_prune
            << ", hit=" << m_hit
            << ", hitRate=" << getHitRate()
            << "}";
        return ss.str();
    }
private:
    int64_t m_get = 0;
    int64_t m_set = 0;
    int64_t m_del = 0;
    int64_t m_timeout = 0;
    int64_t m_prune = 0;        // 记录被淘汰的次数
    int64_t m_hit = 0;    
};



}


}