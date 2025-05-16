#pragma once


#include "socket_stream.h"
#include "cxk/mutex.h"
#include "cxk/util.h"
#include <vector>


namespace cxk{


class LoadBalanceItem{
public:
    using ptr = std::shared_ptr<LoadBalanceItem>;

    virtual ~LoadBalanceItem() {}

    SocketStream::ptr getStream() const { return m_stream; }

    template <class T>
    std::shared_ptr<T> getStreamAs(){
        return std::dynamic_pointer_cast<T>(m_stream);
    }

    virtual int32_t getWeight() { return m_weight; }
    void setWeight(int32_t weight) { m_weight = weight; }

    virtual bool isValid() = 0;

protected:
    SocketStream::ptr m_stream;
    int32_t m_weight = 0;
};


class LoadBalance{
public:
    using RWMutexType = cxk::RWMutex;
    using ptr = std::shared_ptr<LoadBalance>;

    virtual ~LoadBalance() {}

    virtual LoadBalanceItem::ptr get() = 0;
    void add(LoadBalanceItem::ptr v);
    void del(LoadBalanceItem::ptr v);
    void set(const std::vector<LoadBalanceItem::ptr>& vs);

protected:
    RWMutexType m_mutex;
    std::vector<LoadBalanceItem::ptr> m_items;
};


class RoundRobinLoadBalance : public LoadBalance{
public:
    using ptr = std::shared_ptr<RoundRobinLoadBalance>;
    virtual LoadBalanceItem::ptr get() override;
};


class WeightLoadBalance : public LoadBalance{
public:
    using ptr = std::shared_ptr<WeightLoadBalance>;
    virtual LoadBalanceItem::ptr get() override;

    int32_t initWeight();

private:
    int32_t getIdx();

private:
    std::vector<int32_t> m_weights;
};



class HolderStatsSet;

class HolderStats{
friend class HolderStatsSet;
public:
    uint32_t getUsedTime() const { return m_usedTime; }
    uint32_t getTotal() const { return m_total; }
    uint32_t getDoing() const { return m_doing; }
    uint32_t getTimeout() const { return m_timeout; }
    uint32_t getOKs() const { return m_oks; }
    uint32_t getErrs() const { return m_errs; }

    uint32_t incUsedTime(uint32_t v) { return cxk::Atomic::addFetch(m_usedTime ,v);}
    uint32_t incTotal(uint32_t v) { return cxk::Atomic::addFetch(m_total, v);}
    uint32_t incDoing(uint32_t v) { return cxk::Atomic::addFetch(m_doing, v);}
    uint32_t incTimeouts(uint32_t v) { return cxk::Atomic::addFetch(m_timeout, v);}
    uint32_t incOks(uint32_t v) { return cxk::Atomic::addFetch(m_oks, v);}
    uint32_t incErrs(uint32_t v) { return cxk::Atomic::addFetch(m_errs, 0);}

    uint32_t decDoing(uint32_t v) { return cxk::Atomic::subFetch(m_doing, 0);}

    void clear();


    /**
     * @brief 根据成功率计算权重
     *
     * 根据给定的成功率计算权重。
     *
     * @param rate 给定的成功率
     *
     * @return 权重值
     *
     * 计算权重的公式如下：
     * 权重 = (1 - 平均响应时间) * (1 - 超时率 / (总请求数 + 10) * 0.5) * (1 - 进行中请求数 / (总请求数 + 10) * 0.1) * (1 - 错误率 / (总请求数 + 10)) * 成功率
     */
    float getWeight(float rate = 1.0f);

private:
    uint32_t m_usedTime = 0;
    uint32_t m_total = 0;
    uint32_t m_doing = 0;
    uint32_t m_timeout = 0;
    uint32_t m_oks = 0;
    uint32_t m_errs = 0;
};


class HolderStatsSet{
public:
    HolderStatsSet(uint32_t size = 5);
    HolderStats& get(const uint32_t& now = time(0));


    float getWeight(const uint32_t& now = time(0));

private:
    void init(const uint32_t& now = time(0));
private:
    uint32_t m_lastUpdateTime = 0;
    std::vector<HolderStats> m_stats;
};

class FairLoadBalance;
class FairLoadBalanceItem : public LoadBalanceItem{
friend class FairLoadBalance;
public:
    using ptr = std::shared_ptr<FairLoadBalanceItem>;

    void clear();
    virtual int32_t getWeight();
    HolderStats& get(const uint32_t& now = time(0));
protected:
    HolderStatsSet m_stats;
};


class FairLoadBalance : public LoadBalance{
public:
    using ptr = std::shared_ptr<FairLoadBalance>;
    virtual LoadBalanceItem::ptr get() override;

    FairLoadBalanceItem::ptr getAsFair();

    int32_t initWeight();
private:
    int32_t getIdx();
private:
    std::vector<int32_t> m_weights;
};


}