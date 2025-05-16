#include "cxk/cxk.h"
#include <vector>

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void test(){
    std::vector<cxk::Address::ptr> addrs;

    bool v = cxk::Address::Lookup(addrs, "www.baidu.com:80");
    if(!v){
        CXK_LOG_ERROR(g_logger) << "Lookup failed";
        return;
    }
    for(size_t i =0; i < addrs.size(); ++i){
        CXK_LOG_INFO(g_logger) << addrs[i]->toString();
    }
}

void test_iface(){
    std::multimap<std::string , std::pair<cxk::Address::ptr, uint32_t>> results;

    bool v = cxk::Address::GetInterfaceAddresses(results);
    if(!v){
        CXK_LOG_ERROR(g_logger) << "GetInterfaceAddresses failed";
        return;
    }
    for(auto& it : results){
        CXK_LOG_INFO(g_logger) << it.first << " : " << it.second.first->toString() << ":" << it.second.second;
    }
}

void test_ipv4(){
    auto addr = cxk::IPAddress::Create("127.0.0.8");
    if(addr) {
        CXK_LOG_INFO(g_logger) << addr->toString();
    }
}


int main(int argc, char** argv){
    //test();
    //test_iface();
    test_ipv4();
    return 0;
}



