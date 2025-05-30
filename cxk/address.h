#pragma once
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <ifaddrs.h>
#include <vector>
#include <sys/un.h>
#include <map>

namespace cxk{

class IPAddress;


/// @brief  网络地址的基类，抽象类
class Address{
public:
    using ptr = std::shared_ptr<Address> ;


    /// @brief  
    virtual ~Address() {}


    /// @brief          通过sockaddr指针创建Address
    /// @param addr     sockaddr指针
    /// @param addrlen  sockaddr的长度
    /// @return         返回和sockaddr想匹配的Address, 失败返回nullptr
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);  


    /// @brief          通过host地址返回对应条件的所有的Address
    /// @param result   保存满足条件的Address
    /// @param host     域名，服务器名
    /// @param family   协议族
    /// @param type     类型，如SOCK_STREAM, SOCK_DGRAM
    /// @param protocol 协议， IPPROTO_TCP, IPPROTO_UDP
    /// @return         返回是否成功
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, 
        int family = AF_INET, int type = 0, int protocol = 0);


    /// @brief          通过host地址返回对应条件的任意Address
    /// @param result   保存满足条件的Address
    /// @param host     域名，服务器名
    /// @param family   协议族
    /// @param type     类型，如SOCK_STREAM, SOCK_DGRAM
    /// @param protocol 协议， IPPROTO_TCP, IPPROTO_UDP
    /// @return         返回Address， 失败返回nullptr
    static Address::ptr LookupAny(const std::string& host, 
        int family = AF_INET, int type = 0, int protocol = 0);


    /// @brief          通过host地址返回对应条件的任意IPAddress
    /// @param result   保存满足条件的Address
    /// @param host     域名，服务器名
    /// @param family   协议族
    /// @param type     类型，如SOCK_STREAM, SOCK_DGRAM
    /// @param protocol 协议， IPPROTO_TCP, IPPROTO_UDP
    /// @return         返回Address， 失败返回nullptr
    static std::shared_ptr<IPAddress> LookupAnyIpAddr(const std::string& host, 
        int family = AF_INET, int type = 0, int protocol = 0);

    
    /// @brief          返回本机所有网卡的<网卡名，地址，子网掩码位数>
    /// @param result   保存本机所有地址
    /// @param family   协议族
    /// @return         返回是否成功
    static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> & result, 
                int family = AF_INET);
    

    /// @brief          获取制定网卡的地址和子网掩码位数
    /// @param result   保存制定网卡所有地址
    /// @param iface    网卡名称
    /// @param family   协议族（AF_INET, AF_INET6）
    /// @return         返回是否成功
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, 
                const std::string& iface, int family = AF_INET);

    /// @brief  返回协议族
    int getFamily() const;

    /// @brief  返回sockaddr指针，只读
    virtual const sockaddr* getAddr() const = 0;

    /// @brief  返回sockaddr指针, 读写
    virtual sockaddr* getAddr() = 0;

    /// @brief  返回sockaddr的长度
    virtual socklen_t getAddrLen() const = 0;

    /// @brief  可读性输出地址
    virtual std::ostream& insert(std::ostream& os) const = 0;

    /// @brief  返回可读性字符串
    std::string toString() const;

    bool operator < (const Address& rhs)const;
    bool operator == (const Address& rhs)const;
    bool operator != (const Address& rhs) const;
};


/// @brief    IP地址的基类
class IPAddress : public Address {
public:
    using ptr = std::shared_ptr<IPAddress>;

    /// @brief          通过域名，IP，服务器名创建IPAddress
    /// @param address  域名，IP，服务器名等
    /// @param port     端口号
    /// @return         返回IPAddress， 失败返回nullptr
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    /// @brief              获取该地址的广播地址
    /// @param prefix_len   子网掩码位数
    /// @return             调用成功返回返回IPAddress， 失败返回nullptr
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

    /// @brief              获取该地址的网段
    /// @param prefix_len   子网掩码位数
    /// @return             调用成功返回返回IPAddress， 失败返回nullptr
    virtual IPAddress::ptr netWorkAddress(uint32_t prefix_len) = 0;


    /// @brief              获取子网掩码地址
    /// @param prefix_len   子网掩码位数
    /// @return             调用成功返回返回IPAddress， 失败返回nullptr
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;       

    /// @brief  返回端口号
    virtual uint32_t getPort() const = 0;       
    
    
    /// @brief  设置端口号
    virtual void setPort(uint16_t v) = 0;

};



/// @brief    IPv4地址
class IPv4Address : public IPAddress{
public:
    using ptr = std::shared_ptr<IPv4Address>;

    /// @brief          使用点分十进制地址创建IPv4地址
    /// @param address  点分十进制地址
    /// @param port     端口号
    /// @return         返回IPv4Address， 失败返回nullptr
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);


    /// @brief          通过sockaddr_in创建IPv4地址
    /// @param address  sockaddr_in结构体
    IPv4Address(const sockaddr_in& address);


    /// @brief          通过二进制地址创建IPv4地址
    /// @param address  二进制地址
    /// @param port     端口号
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    virtual const sockaddr* getAddr() const override;
    virtual sockaddr* getAddr() override;
    virtual socklen_t getAddrLen() const override;

    virtual std::ostream& insert(std::ostream& os) const override;
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;   
    virtual IPAddress::ptr netWorkAddress(uint32_t prefix_len) override;     
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) override;         

    virtual uint32_t getPort() const override;       
    virtual void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};



/// @brief    IPv6地址
class IPv6Address : public IPAddress{
public:
    using ptr = std::shared_ptr<IPv6Address>;

    /// @brief  无参构造函数
    IPv6Address();

    /// @brief          通过sockaddr_in6构造IPv6地址
    /// @param address  sockaddr_in6结构体
    IPv6Address(const sockaddr_in6& address);


    /// @brief          通过二进制地址构造IPv6地址
    /// @param address  二进制地址
    /// @param port     端口号
    IPv6Address(const uint8_t address[16], uint16_t port);


    /// @brief          通过IPv6地址字符串构造IPv6地址
    /// @param address  IPv6地址字符串
    /// @param port     端口号
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    virtual const sockaddr* getAddr() const override;
    virtual sockaddr* getAddr() override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const override;

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;   // 获取广播地址
    virtual IPAddress::ptr netWorkAddress(uint32_t prefix_len) override;     // 获取网络地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) override;         // 获取子网掩码

    virtual uint32_t getPort() const override;       
    virtual void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;
};


/// @brief    Unix域套接字地址
class UnixAddress : public Address{
public:
    using ptr = std::shared_ptr<UnixAddress>;
    
    /// @brief      通过路径创建Unix域套接字地址
    /// @param path UnixSocket路径
    UnixAddress(const std::string& path);

    /// @brief  无参构造函数
    UnixAddress();

    void setAddrLen(uint32_t v);
    std::string getPath() const;

    virtual const sockaddr* getAddr() const override;
    virtual sockaddr* getAddr() override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const override;
private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};


/// @brief   未知地址
class UnknownAddress : public Address{
public:
    using ptr = std::shared_ptr<UnknownAddress>;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& address);
        
    virtual const sockaddr* getAddr() const override;
    virtual sockaddr* getAddr() override;
    virtual socklen_t getAddrLen() const override;
    virtual std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};


/// @brief  流式输出地址
std::ostream& operator<<(std::ostream& os, const Address& addr);


}