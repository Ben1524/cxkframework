#include "address.h"
#include "endian.h"
#include "logger.h"
#include <sstream>
#include <netdb.h>
#include <vector>


namespace cxk{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

template <class T>
static T createMask(uint32_t bits){
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}



// 统计二进制中1的个数
template<class T>
static uint32_t CountBytes(T value){
    uint32_t result = 0;
    for(; value; ++result){
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen){
    if(addr == nullptr){
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family){
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
    }

    return result;
}

/* 执行DNS查找，以获取给定主机名（可能是IPv4或IPv6地址）的网络地址信息
    1. 首先，检查主机名是否以[开头，这通常表示一个IPv6地址。如果是，则提取地址和可能的服务名。
    2. 如果不是IPv6地址，则检查是否存在冒号:，这通常用于分隔主机名和服务名（如端口号）。
    3. 如果都没有找到，则假设整个字符串都是主机名。
 */
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host, 
        int family, int type, int protocol){
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char* service = NULL;

    // 检查 ipv6address serive
    if(!host.empty() && host[0] == '['){
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6){
            // TODO check out of range
            if(*(endipv6 + 1) == ':'){
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    // 检查 node service
    if(node.empty()){
        service = (const char*) memchr(host.c_str(), ':', host.size());
        if(service){
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)){
                node = host.substr(0, service - host.c_str());
                ++ service;
            }
        }
    }

    if(node.empty()){
        node = host;
    }

    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        CXK_LOG_ERROR(g_logger)  << "Address::Lookup failed";
        return false;
    }

    next = results;
    while(next) {
        result.push_back(Address::Create(next->ai_addr,(socklen_t) next->ai_addrlen));
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return !result.empty();
}


// 查找随意一个地址
Address::ptr Address::LookupAny(const std::string& host, 
        int family , int type, int protocol){
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)){
        return result[0];
    }
    return nullptr;
}

 
// 随意查找一个IP地址
IPAddress::ptr Address::LookupAnyIpAddr(const std::string& host, 
        int family , int type , int protocol ){
    std::vector<Address::ptr> result;
    if(Address::Lookup(result, host, family, type, protocol)){
        for(auto& i : result){
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v){
                return v;
            }
        }
    }
    return nullptr;     
}


bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> & result, 
                int family){
    struct ifaddrs* next, *results;
    if(getifaddrs(&results) != 0){
        CXK_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs error";
        return false;
    }

    try{
        for(next = results; next; next = next->ifa_next){
            Address::ptr addr;
            uint32_t prefix_length = ~0u;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family){
                continue;
            }
            switch (next->ifa_addr->sa_family)
            {
            case AF_INET:
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_length = CountBytes(netmask);
                }
                break;
            case AF_INET6:
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                    prefix_length = 0;
                    for(int i = 0; i < 16; i++){
                        prefix_length += CountBytes(netmask.s6_addr[i]);
                    }
                }
                break;
            default:
                break;
            }

            if(addr){
                result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_length)));
            }
        }
    } catch(...){
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !result.empty();
}
    

bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, 
                const std::string& iface, int family){
    if(iface.empty() || iface == "*"){
        if(family == AF_INET || family == AF_UNSPEC){
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

    if(!GetInterfaceAddresses(results, family)){
        return false;
    }

    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first){
        result.push_back(its.first->second);
    }
    return !result.empty();
}



int Address::getFamily() const{
    return getAddr()->sa_family;
}

std::string Address::toString() const{
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator < (const Address& rhs)const{
    socklen_t minLen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minLen);

    if(result < 0){
        return true;
    } else if(result > 0){
        return false;
    } else if(getAddrLen() < rhs.getAddrLen()){
        return true;
    }
    return false;
}


bool Address::operator == (const Address& rhs)const{
    return getAddrLen() == rhs.getAddrLen() 
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}


bool Address::operator != (const Address& rhs) const{
    return !(*this == rhs);
}


IPAddress::ptr IPAddress::Create(const char* address, uint16_t port){
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    /* 调用getaddrinfo函数来解析提供的IP地址。如果解析成功，results将指向一个包含解析结果的链表。*/
    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error){
        CXK_LOG_ERROR(g_logger) << "IPAddress::Create failed";
        return nullptr;
    }

    try{
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, results->ai_addrlen));
        if(result){
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    }catch(...){
        freeaddrinfo(results);
        return nullptr;
    }
}


IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port){
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0){
        CXK_LOG_ERROR(g_logger) << "Ipv6Address create failed";
        return nullptr;
    }
    return rt;
}



IPv4Address::IPv4Address(uint32_t address , uint16_t port ){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);     //转成网络字节序
}

const sockaddr* IPv4Address::getAddr() const{
    return (sockaddr*)&m_addr;
}

sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&m_addr;
}
 
 
socklen_t IPv4Address::getAddrLen() const{
    return sizeof(m_addr);
}


std::ostream& IPv4Address::insert(std::ostream& os) const{

    {
        const sockaddr_in* addr_in = &m_addr;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr_in->sin_addr), ip, INET_ADDRSTRLEN);
        int port = ntohs(addr_in->sin_port);

        std::cout << "IP: " << ip << std::endl;
        std::cout << "Port: " << port << std::endl;
    }

    uint32_t addr = ntohl(m_addr.sin_addr.s_addr);
    os << ( (addr >> 24) & 0xFF ) << '.'
        << ( (addr >> 16) & 0xFF ) << '.'
        << ( (addr >> 8) & 0xFF ) << '.'
        << ( addr & 0xFF );
    os << ':' << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}


IPv4Address::IPv4Address(const sockaddr_in& address){
    m_addr = address;
}


IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len){
    if(prefix_len > 32){
        return nullptr;
    }

    sockaddr_in addr(m_addr);
    addr.sin_addr.s_addr |= byteswapOnLittleEndian(createMask<uint32_t>(prefix_len));

    return IPv4Address::ptr(new IPv4Address(addr));
}


IPAddress::ptr IPv4Address::netWorkAddress(uint32_t prefix_len){
    if(prefix_len > 32){
        return nullptr;
    }

    sockaddr_in addr(m_addr);
    addr.sin_addr.s_addr &= byteswapOnLittleEndian(createMask<uint32_t>(prefix_len));

    return IPv4Address::ptr(new IPv4Address(addr));
}


IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len){
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(createMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}



uint32_t IPv4Address::getPort() const{
    return byteswapOnLittleEndian(m_addr.sin_port);
}   


void IPv4Address::setPort(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}


IPv6Address::IPv6Address(){

}


IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr, address, 16);
}



IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port){
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin6_addr);
    if(result <= 0){
        CXK_LOG_ERROR(g_logger) << "Ipv4Address create failed";
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address(const sockaddr_in6& address){
    m_addr = address;
}


const sockaddr* IPv6Address::getAddr() const{
    return (sockaddr*)&m_addr;
}

sockaddr* IPv6Address::getAddr() {
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const{
    return sizeof(m_addr);
}


std::ostream& IPv6Address::insert(std::ostream& os) const{
    os << "[";
    uint16_t* addr =  (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;

    for(size_t i = 0; i < 8; ++i){
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i-1] == 0 && !used_zeros){
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int) byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if(!used_zeros && addr[7] == 0){
        os << ":";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}


IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |= createMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; i++){
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr (new IPv6Address(baddr));
}


IPAddress::ptr IPv6Address::netWorkAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &= createMask<uint8_t>(prefix_len % 8);
    // for(int i = prefix_len / 8 + 1; i < 16; i++){
    //     baddr.sin6_addr.s6_addr[i] = 0xff;
    // }
    return IPv6Address::ptr (new IPv6Address(baddr));
}


IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len / 8] = ~createMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; i++){
        subnet.sin6_addr.s6_addr[i] = 0xFF;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}       


void IPv6Address::setPort(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}


static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress(){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    if(!path.empty() && path[0] == '\0'){
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)){
        throw std::logic_error("path is too long");
    }

    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length = offsetof(sockaddr_un, sun_path) + m_length;
}


std::string UnixAddress::getPath() const{
    std::stringstream ss;
    if(m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0'){
        ss << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}


const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

sockaddr* UnixAddress::getAddr(){
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}


std::ostream& UnixAddress::insert(std::ostream& os) const{
    if(m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0'){
        return os << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

void UnixAddress::setAddrLen(uint32_t v){
    m_length = v;
}



UnknownAddress::UnknownAddress(int family){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& address){
    m_addr = address;
}


const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}

sockaddr* UnknownAddress::getAddr(){
    return &m_addr;
}


socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);
}


std::ostream& UnknownAddress::insert(std::ostream& os) const {
    os << "[UnknownAddress]";
    return os;
}


std::ostream& operator<<(std::ostream& os, const Address& addr){
    return addr.insert(os);
}


}