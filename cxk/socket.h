#pragma once
#include <memory>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "address.h"
#include "macro.h"
#include "noncopyable.h"
#include <openssl/ssl.h>
#include <openssl/err.h>


namespace cxk{

/// @brief Socket 封装类
class Socket: public std::enable_shared_from_this<Socket>, Noncopyable{
public:
    using ptr = std::shared_ptr<Socket>;
    using weak_ptr = std::weak_ptr<Socket>;

    /// @brief Socket 类型
    enum Type{
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    /// @brief Socket 协议族
    enum Family{
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        Unix = AF_UNIX
    };


    /// @brief      创建TCP Socket（满足地址类型）
    /// @param addr 地址
    static Socket::ptr CreateTCP(cxk::Address::ptr addr);


    /// @brief      创建UDP Socket（满足地址类型）
    /// @param addr 地址
    static Socket::ptr CreateUDP(cxk::Address::ptr addr);


    /// @brief      创建IPv4的TCP Socket
    static Socket::ptr CreateTCPSocket();

    /// @brief      创建IPv4的UDP Socket
    static Socket::ptr CreateUDPSocket();


    /// @brief      创建IPv6的TCP Socket
    static Socket::ptr CreateTCPSocket6();

    /// @brief      创建IPv6的UDP Socket
    static Socket::ptr CreateUDPSocket6();


    /// @brief      创建Unix域的TCP Socket
    static Socket::ptr CreateUnixTCPSocket();

    /// @brief      创建Unix域的UDP Socket
    static Socket::ptr CreateUnixUDPSocket();


    /// @brief          构造函数
    /// @param family   协议族
    /// @param type     类型
    /// @param protocol 协议
    Socket(int family, int type, int protocol = 0);

    /// @brief          析构函数
    virtual ~Socket();


    /// @brief  获取发送超时时间
    int64_t getSendTimeout();

    /// @brief  设置发送超时时间
    void setSendTimeout(int64_t timeout);


    /// @brief  获取接收超时时间
    int64_t getRecvTimeout();

    /// @brief  设置接收超时时间
    void setRecvTimeout(int64_t timeout);

    /// @brief          获取sockopt @see getsockopt
    bool getOption(int level, int option, void* result, size_t* len);


    /// @brief          获取sockopt模板 @see getsockopt
    template<class T>
    bool getOption(int level, int option, T& result) {
        size_t len = sizeof(T);
        return getOption(level, option, &result, &len);
    }


    /// @brief          设置sockopt @see setsockopt
    bool setOption(int level, int option, const void* data, size_t len);
    
    /// @brief      设置sockopt模板 @see setsockopt
    template<class T>
    bool setOption(int level, int option, const T& data) {
        return setOption(level, option, &data, sizeof(T));
    }


    /// @brief  接受connect连接
    /// @return 成功返回新连接的socket， 失败返回nullptr
    /// @pre    Socket必须bind， listen成功
    virtual Socket::ptr accept();


    /// @brief      绑定地址
    /// @param addr 地址
    /// @return     成功返回true，失败返回false
    virtual bool bind(const Address::ptr addr);


    /// @brief              连接地址
    /// @param addr         地址
    /// @param timeout_ms   超时时间
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);


    virtual bool reconnect(uint64_t timeout_ms = -1);


    /// @brief          监听socket
    /// @param backlog  未完成连接队列长度
    /// @return         成功返回true，失败返回false
    /// @pre            必须先bind成功
    virtual bool listen(int backlog = SOMAXCONN);

    /// @brief          关闭socket
    virtual bool close();


    /// @brief          发送数据
    /// @param buffer   待发送数据的内存
    /// @param length   待发送数据长度
    /// @param flags    发送标志
    /// @return         @retval >0 成功发送的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int send(const void* buffer, size_t length, int flags = 0);


    /// @brief          发送数据
    /// @param buffers  待发送数据内存(iovec数组)
    /// @param count    待发送数据长度
    /// @param flags    发送标志
    /// @return         @retval >0 成功发送的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int send(const iovec* buffers, size_t count, int flags = 0);

    /// @brief          发送数据
    /// @param buffer   待发送数据内存
    /// @param length   待发送数据长度
    /// @param addr     地址
    /// @param flags    发送标志
    /// @return         @retval >0 成功发送的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int sendto(const void* buffer, size_t length, const Address::ptr addr, int flags = 0);


    /// @brief          发送数据
    /// @param buffers  待发送数据内存(iovec数组)
    /// @param count    待发送数据长度
    /// @param addr     地址
    /// @param flags    发送标志
    /// @return         @retval >0 成功发送的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int sendto(const iovec* buffers, size_t count, const Address::ptr addr, int flags = 0);


    /// @brief          接收数据
    /// @param buffer   待接收数据的内存
    /// @param length   待接收数据长度
    /// @param flags    接收标志
    /// @return         @retval >0 成功接收的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int recv(void* buffer, size_t length, int flags = 0);

    /// @brief          接收数据
    /// @param buffers  待接收数据内存(iovec数组)
    /// @param count    待接收数据长度
    /// @param flags    接收标志
    /// @return         @retval >0 成功接收的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int recv(iovec* buffers, size_t count, int flags = 0);

    /// @brief          接收数据
    /// @param buffer   待接收数据的内存
    /// @param length   待接收数据长度
    /// @param addr     地址
    /// @param flags    接收标志
    /// @return         @retval >0 成功接收的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int recvfrom(void* buffer, size_t length, Address::ptr addr, int flags = 0);

    /// @brief          接收数据
    /// @param buffers  待接收数据内存(iovec数组)
    /// @param count    待接收数据长度
    /// @param addr     地址
    /// @param flags    接收标志
    /// @return         @retval >0 成功接收的字节数， @retval =0 对方关闭连接， @retval <0 出错
    virtual int recvfrom(iovec* buffers, size_t count, Address::ptr addr, int flags = 0);

    /// @brief  获取远端地址
    Address::ptr getRemoteAddress();

    /// @brief  获取本地地址
    Address::ptr getLocalAddress();

    /// @brief  获取协议簇
    int getFamily() const {return m_family;}

    /// @brief  获取类型
    int getType() const {return m_type;}
    
    /// @brief  获取协议
    int getProtocol() const {return m_protocol;}

    /// @brief  是否已经连接
    int isConnected() const {return m_isConnected;}

    /// @brief  是否有效(m_socket != -1)
    bool isValid() const;

    /// @brief  获取错误码
    int getError();


    /// @brief  信息输出到流中
    virtual std::ostream& dump(std::ostream& os) const;

    virtual std::string toString() const;

    /// @brief  获取socket句柄
    int getSokcet() const {return m_socket;}

    /// @brief  取消读
    bool cancelRead();

    /// @brief  取消写
    bool cancelWrite();
    
    /// @brief  取消连接
    bool cancelAccept();
    
    /// @brief  取消所有事件
    bool cancelAll();

protected:
    /// @brief  初始化socket
    void initSock(){
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        if(m_type == SOCK_STREAM){
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }


    /// @brief  创建socket
    void newSock(){
        m_socket = ::socket(m_family, m_type , m_protocol);
        if(CXK_LICKLY( m_socket != -1 )){
            initSock();
        }
    }
    

    /// @brief  初始化socket
    virtual bool init(int sock);

protected:
    int m_socket;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;

    Address::ptr m_localAddr;
    Address::ptr m_remoteAddr;
};



class SSLSocket : public Socket{
public:
    using ptr = std::shared_ptr<SSLSocket>;

    static SSLSocket::ptr CreateTCP(cxk::Address::ptr address);
    static SSLSocket::ptr CreateTCPSocket();
    static SSLSocket::ptr CreateTCPSocket6();

    SSLSocket(int family, int type, int protocol = 0);
    virtual Socket::ptr accept() override;
    virtual bool bind(const Address::ptr addr) override;
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
    virtual bool listen(int backlog = SOMAXCONN) override;
    virtual bool close() override;
    virtual int send(const void* buffer, size_t length, int flags = 0) override;
    virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
    virtual int sendto(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int sendto(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int recv(void* buffer, size_t length, int flags = 0) override;
    virtual int recv(iovec* buffers, size_t length, int flags = 0) override;
    virtual int recvfrom(void* buffer, size_t length, Address::ptr from, int flags = 0) override;
    virtual int recvfrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0) override;

    /**
     * @brief 加载SSL证书和私钥
     *
     * 该函数用于加载SSL证书和私钥，并设置到SSL上下文中。
     *
     * @param cert_file 证书文件的路径
     * @param key_file 私钥文件的路径
     * @return 加载成功返回true，否则返回false
     */
    bool loadCertificates(const std::string& cert_file, const std::string& key_file);
    virtual std::ostream& dump(std::ostream& os) const override;

protected:
    virtual bool init(int sock) override;
private:
    std::shared_ptr<SSL_CTX> m_ctx;
    std::shared_ptr<SSL> m_ssl;
};





/// @brief      流式输出socket
/// @param os   输出流
/// @param sock Socket类
std::ostream& operator<<(std::ostream& os, Socket::ptr sock);



}