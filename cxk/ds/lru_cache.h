#pragma once


#include <algorithm>
#include <list>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <sstream>
#include "cache_status.h"
#include "cxk/mutex.h"


namespace cxk{


namespace ds{

template <class K, class V, class MutexType = cxk::Mutex>
class LruCache{
public:
    using ptr = std::shared_ptr<LruCache>;

    using item_type = std::pair<K, V>;
    using list_type = std::list<item_type>;
    using map_type = std::unordered_map<K, typename list_type::iterator> ;
    using prune_callback = std::function<void(const K&, const V&)> ;

    LruCache(size_t max_size = 0, size_t elasticity = 0, CacheStatus* status = nullptr) : m_maxSize(max_size), m_elasticity(elasticity) {
        m_status = status;
        if(status == nullptr){
            m_status = new CacheStatus;
            m_statusOwner = true;
        }
    }


    ~LruCache(){
        if(m_statusOwner && m_status){
            delete m_status;
        }
    }


    void set(const K& k, const V& v){
        m_status->incSet();
        typename MutexType::Lock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it != m_cache.end()){
            it->second->second = v;
            m_keys.splice(m_keys.begin(), m_keys, it->second);
            return;
        }
        m_keys.emplace_front(std::make_pair(k, v));
        m_cache.insert(std::make_pair(k, m_keys.begin()));
        prune();
    }


    bool get(const K& k, V& v){
        m_status->incGet();
        typename MutexType::Lock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()){
            return false;
        }

        m_keys.splice(m_keys.begin(), m_keys, it->second);
        v = it->second->second;
        lock.unlock();
        m_status->incHit();
        return v;
    }


    bool del(const K& k){
        m_status->incDel();
        typename MutexType::Lock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()){
            return false;
        }

        m_keys.erase(it->second);
        m_cache.erase(it);
        return true;
    }

    bool exists(const K& k){
        typename MutexType::Lock lock(m_mutex);
        return m_cache.find(k) != m_cache.end();
    }

    size_t size(){
        typename MutexType::Lock lock(m_mutex);
        return m_cache.size();
    }


    bool empty(){
        typename MutexType::Lock lock(m_mutex);
        return m_cache.empty();
    }

    bool clear(){
        typename MutexType::Lock lock(m_mutex);
        m_cache.clear();
        m_keys.clear();
        return true;
    }

    void setMaxSize(size_t max_size){
        m_maxSize = max_size;
    }


    void setElasticity(size_t elasticity){
        m_elasticity = elasticity;
    }


    size_t getMaxSize() const {
        return m_maxSize;
    }

    size_t getElasticity() const {
        return m_elasticity;
    }

    size_t getMaxAllowedSize() const {
        // 返回最大允许大小，即最大固定大小加上弹性大小
        return m_maxSize + m_elasticity;
    }


    template <class F>
    void foreach(F& f){
        typename MutexType::Lock lock(m_mutex);
        std::for_each(m_cache.begin(), m_cache.end(), f);
    }

    void setPruneCallback(prune_callback cb){
        m_cb = cb;
    }

    std::string toStatusString(){
        std::stringstream ss;
        ss << (m_status ? m_status->toString() : "(no status)" ) << "total = " << size();
        return ss.str();
    }

    CacheStatus* getStatus() const { return m_status;}

    void setStatus(CacheStatus* v, bool owner = false){
        if(m_statusOwner && m_status){
            delete m_status;
        }

        m_status = v;
        m_statusOwner = owner;

        if(m_status == nullptr){
            m_status = new CacheStatus;
            m_statusOwner = true;
        }
    }

protected:
    size_t prune(){
        if(m_maxSize == 0 || m_cache.size() < getMaxAllowedSize()){
            return 0;
        }

        size_t count = 0;
        while(m_cache.size() > m_maxSize){
            auto& back = m_keys.back();
            if(m_cb){
                m_cb(back.first, back.second);
            }
            m_cache.erase(back.first);
            m_keys.pop_back();
            ++count;
        }
        m_status->incPrune(count);
        return count;
    }

private:
    MutexType m_mutex;
    map_type m_cache;
    list_type m_keys;
    size_t m_maxSize;
    size_t m_elasticity;
    prune_callback m_cb;
    CacheStatus* m_status = nullptr;
    bool m_statusOwner = false;

};


template<class K, class V, class MutexType = cxk::Mutex, class Hash = std::hash<K>>
class HashLruCache {
public:
    using ptr = std::shared_ptr<HashLruCache>;
    using cache_type = LruCache<K, V, MutexType>;

    HashLruCache(size_t bucket, size_t max_size, size_t m_elasticity) : m_bucket(bucket) { 
        m_datas.resize(bucket);

        size_t pre_max_size = std::ceil(max_size * 1.0 / bucket);
        size_t pre_elasiticity = std::ceil(m_elasticity * 1.0 / bucket);
        m_maxSize = pre_max_size * bucket;
        m_elasticity = pre_elasiticity * bucket;

        for(size_t i = 0; i < bucket; ++i){
            m_datas[i] = new cache_type(pre_max_size, pre_elasiticity, &m_status);
        }
    }


    ~HashLruCache(){
        for(size_t i = 0; i < m_datas.size(); ++i){
            delete m_datas[i];
        }
    }

    void set(const K& k, const V& v) {
        m_datas[m_hash(k) % m_bucket]->set(k, v);
    }

    bool get(const K& k, V& v) {
        return m_datas[m_hash(k) % m_bucket]->get(k, v);
    }

    V get(const K& k) {
        return m_datas[m_hash(k) % m_bucket]->get(k);
    }

    bool del(const K& k) {
        return m_datas[m_hash(k) % m_bucket]->del(k);
    }

    bool exists(const K& k) {
        return m_datas[m_hash(k) % m_bucket]->exists(k);
    }

    size_t size(){
        size_t total = 0;
        for(auto& i : m_datas){
            total += i->size();
        }
        return total;
    }

    bool empty(){
        for(auto& i : m_datas){
            if(!i->empty()){
                return false;
            }
        }

        return true;
    }


    void clear(){
        for(auto& i : m_datas){
            i->clear();
        }
    }


    size_t getMaxSize() const { return m_maxSize;}
    size_t getElasticity() const { return m_elasticity;}
    size_t getMaxAllowedSize() const { return m_maxSize + m_elasticity;}
    size_t getBucket() const { return m_bucket;}


    void setMaxSize(const size_t& v){
        size_t pre_max_size = std::ceil(v * 1.0 / m_bucket);
        m_maxSize = pre_max_size * m_bucket;
        for(auto& i : m_datas){
            i->setMaxSize(pre_max_size);
        }
    }


    void setElasticity(const size_t& v){
        size_t pre_elasticity = std::ceil(v * 1.0 / m_bucket);
        m_elasticity = pre_elasticity * m_bucket;
        for(auto& i : m_datas){
            i->setElasticity(pre_elasticity);
        }
    }


    template<class F>
    void foreach(F& f){
        for(auto& i : m_datas){
            i->foreach(f);
        }
    }

    void setPruneCallback(typename cache_type::prune_callback cb){
        for(auto& i : m_datas){
            i->setPruneCallback(cb);
        }
    }

    CacheStatus* getStatus(){
        return &m_status;
    }

    std::string toStatusString(){
        std::stringstream ss;
        ss << m_status.toString() << " total = " << size();
        return ss.str();
    }

private:
    std::vector<cache_type*> m_datas;
    size_t m_maxSize;
    size_t m_bucket;
    size_t m_elasticity;
    Hash m_hash;
    CacheStatus m_status;
};




}



}