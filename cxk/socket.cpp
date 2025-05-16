#include "socket.h"
#include "fd_manager.h"
#include "logger.h"
#include "hook.h"
#include "iomanager.h"


namespace cxk{

// 声明该函数，否则编译的时候找不到

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");


Socket::ptr Socket::CreateTCP(cxk::Address::ptr addr){
    Socket::ptr socket (new Socket(addr->getFamily(), TCP, 0));
    if(!socket){
        CXK_LOG_ERROR(g_logger) << "CreateTCP socket failed";
    }
    return socket;
}


Socket::ptr Socket::CreateUDP(cxk::Address::ptr addr){
    Socket::ptr socket (new Socket(addr->getFamily(), UDP, 0));
    socket->newSock();
    socket->m_isConnected = true;
    return socket;
}

Socket::ptr Socket::CreateTCPSocket(){
    Socket::ptr socket(new Socket(IPv4, TCP, 0));
    return socket;
}

Socket::ptr Socket::CreateUDPSocket(){
    Socket::ptr socket(new Socket(IPv4, UDP, 0));
    socket->newSock();
    socket->m_isConnected = true;
    return socket;
}

Socket::ptr Socket::CreateTCPSocket6(){
    Socket::ptr socket(new Socket(IPv6, TCP, 0));
    return socket;
}



Socket::ptr Socket::CreateUDPSocket6(){
    Socket::ptr socket(new Socket(IPv6, UDP, 0));
    socket->newSock();
    socket->m_isConnected = true;
    return socket;
}

Socket::ptr Socket::CreateUnixTCPSocket(){
    Socket::ptr socket(new Socket(Unix, TCP, 0));
    return socket;
}

Socket::ptr Socket::CreateUnixUDPSocket(){
    Socket::ptr socket(new Socket(Unix, UDP, 0));
    return socket;
}

Socket::Socket(int family, int type, int protocol) : m_socket(-1), 
    m_family(family), m_type(type), m_protocol(protocol), m_isConnected(false){

}

Socket::~Socket(){
    close(); 
}

int64_t Socket::getSendTimeout(){
    FdCtx::ptr ctx = FdMgr::GetInstance() ->get(m_socket);
    if(ctx) {
        return ctx->getTimeOut(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(int64_t timeout){
    struct timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout(){
    FdCtx::ptr ctx = FdMgr::GetInstance() ->get(m_socket);
    if(ctx) {
        return ctx->getTimeOut(SO_RCVTIMEO);
    }
    return -1;
}
void Socket::setRecvTimeout(int64_t timeout){
    struct timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, size_t* len){
    int rt = getsockopt(m_socket, level, option, result, (socklen_t*)len);
    if(rt){
        CXK_LOG_DEBUG(g_logger) << "getsockopt failed, errno: " << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void* data, size_t len){
    if(setsockopt(m_socket, level, option, data, (socklen_t)len)){
        CXK_LOG_DEBUG(g_logger) << "setsockopt failed, errno: " << strerror(errno) << ", level: " << level << ", option: " << option;
        return false;
    }
    return true;
}


// 接受一个连接
Socket::ptr Socket::accept(){
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newSock = ::accept(m_socket, NULL, NULL);
    if(newSock == -1){
        CXK_LOG_DEBUG(g_logger) << "accept failed, errno: " << strerror(errno);
        return nullptr;
    }
    if(sock->init(newSock)){
        return sock;
    }
    return nullptr;
}


// 初始化socket
bool Socket::init(int sock){
    FdCtx::ptr ctx = FdMgr::GetInstance() ->get(sock);
    if(ctx == nullptr){
        CXK_LOG_DEBUG(g_logger) << "fd not exist";
    }
    if(ctx && ctx->isSocket() && !ctx->isClose()) {
        m_socket = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }

    return false;
}


// 将socket绑定到对应的地址
bool Socket::bind(const Address::ptr addr){
    m_localAddr = addr;
    if(CXK_UNLIKELY(!isValid())){
        newSock();
        if(CXK_UNLIKELY(!isValid())){
            return false;
        }
    }

    if(CXK_UNLIKELY(addr->getFamily() != m_family)){
        CXK_LOG_ERROR(g_logger) << "family not match";
        return false;
    }   

    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if(uaddr){
        Socket::ptr sock = Socket::CreateUnixUDPSocket();
        if(sock->connect(uaddr)){
            return false;
        } else {
            cxk::FSUtil::Unlink(uaddr->getPath(), true);
        }
    }

    if(::bind(m_socket, addr->getAddr(), addr->getAddrLen())){
        CXK_LOG_ERROR(g_logger) << "bind failed, errno: " << strerror(errno);
        return false;
    }

    getLocalAddress();
    return true;
}


bool Socket::reconnect(uint64_t timeout_ms){
    if(!m_remoteAddr){
        CXK_LOG_ERROR(g_logger) << "remote address is null";
        return false;
    }
    m_localAddr.reset();
    return connect(m_remoteAddr, timeout_ms);
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms){
    m_remoteAddr = addr;
    if(CXK_UNLIKELY(!isValid())){
        newSock();
        if(CXK_UNLIKELY(!isValid())){
            return false;
        }
    }

    if(CXK_UNLIKELY(addr->getFamily() != m_family)){
        CXK_LOG_ERROR(g_logger) << "connect family not match";
        return false;
    }   

    if(timeout_ms == (uint64_t)-1){
        if(::connect(m_socket, addr->getAddr(), addr->getAddrLen())){
            CXK_LOG_ERROR(g_logger) << "connect failed, errno: " << strerror(errno);
            return false;
        }
    } else {
        if(::connect_with_timeout(m_socket, addr->getAddr(), addr->getAddrLen(), timeout_ms)){
            CXK_LOG_ERROR(g_logger) << "connect_with_timeout failed, errno: " << strerror(errno);
            return false;
        }
    }

    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

bool Socket::listen(int backlog){
    if(!isValid()){
        CXK_LOG_ERROR(g_logger) << "socket is not valid";
        return false;
    }
    if(::listen(m_socket, backlog)){
        CXK_LOG_ERROR(g_logger) << "listen failed, errno: " << strerror(errno);
        return false;
    }

    return true;
}


// 关闭socket
bool Socket::close(){
    if(!m_isConnected && m_socket == -1){
        return true;
    }

    m_isConnected = false;
    if(m_socket != -1){
        ::close(m_socket);
        m_socket = -1;
    }
    return false;
}

// send 发送数据
int Socket::send(const void* buffer, size_t length, int flags){
    if(isConnected()){
        return ::send(m_socket, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const iovec* buffers, size_t count, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msghdr));
        msg.msg_iov = (struct iovec*)buffers;
        msg.msg_iovlen = count;
        return ::sendmsg(m_socket, &msg, flags);
    }

    return -1;
}

int Socket::sendto(const void* buffer, size_t length, const Address::ptr addr, int flags){
    if(isConnected()){
        return ::sendto(m_socket, buffer, length, flags, addr->getAddr(), addr->getAddrLen());
    }
    return -1;
}

int Socket::sendto(const iovec* buffers, size_t count, const Address::ptr addr, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msghdr));
        msg.msg_iov = (struct iovec*)buffers;
        msg.msg_iovlen = count;
        // msg.msg_name = addr.getAddr();
        msg.msg_namelen = addr->getAddrLen();
        return ::sendmsg(m_socket, &msg, flags);
    }

    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags){
    if(isConnected()){
        int rt = ::recv(m_socket, buffer, length, flags);
        CXK_LOG_DEBUG(g_logger) << "Socket::recv: " << rt;
        return rt;
    }
    return -1;
}

int Socket::recv(iovec* buffers, size_t count, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msghdr));
        msg.msg_iov = (struct iovec*)buffers;
        msg.msg_iovlen = count;
        return ::recvmsg(m_socket, &msg, flags);
    }
    return -1;
}

int Socket::recvfrom(void* buffer, size_t length, Address::ptr addr, int flags){
    if(isConnected()){
        socklen_t len = addr->getAddrLen();
        return ::recvfrom(m_socket, buffer, length, flags, addr->getAddr(), &len);
    }
    return -1;
}

int Socket::recvfrom(iovec* buffers, size_t count, Address::ptr addr, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msghdr));
        msg.msg_iov = (struct iovec*)buffers;
        msg.msg_iovlen = count;
        // msg.msg_name = addr.getAddr();
        msg.msg_namelen = addr->getAddrLen();
        return ::recvmsg(m_socket, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress(){
    if(m_remoteAddr){
        return m_remoteAddr;
    }

    Address::ptr result;
    switch (m_family)
    {
    case AF_INET:
        result.reset(new IPv4Address());
        break;
    case AF_INET6:
        result.reset(new IPv6Address());
        break;
    case AF_UNIX:
        result.reset(new UnixAddress());
        break;
    default:
        result.reset(new UnknownAddress(m_family));
        break;
    }

    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_socket, result->getAddr(), &addrlen)){
        CXK_LOG_ERROR(g_logger) << "getpeername failed: " << strerror(errno);
        return Address::ptr (new UnknownAddress(m_family));
    }

    if(m_family == AF_UNIX){
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddr = result;
    return m_remoteAddr;
}

Address::ptr Socket::getLocalAddress(){
    if(m_localAddr){
        return m_localAddr;
    }

    Address::ptr result;
    switch (m_family)
    {
    case AF_INET:
        result.reset(new IPv4Address());
        break;
    case AF_INET6:
        result.reset(new IPv6Address());
        break;
    case AF_UNIX:
        result.reset(new UnixAddress());
        break;
    default:
        result.reset(new UnknownAddress(m_family));
        break;
    }

    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_socket, result->getAddr(), &addrlen)){
        CXK_LOG_ERROR(g_logger) << "getsockname failed: " << strerror(errno);
        return Address::ptr (new UnknownAddress(m_family));
    }

    if(m_family == AF_UNIX){
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddr = result;
    return m_localAddr;
}


// 该socket是否有效
bool Socket::isValid() const{
    return m_socket != -1;
}

int Socket::getError(){
    int error = 0;
    size_t len = sizeof(error);

    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)){
        return -1;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const{
    os << "[Socket sock=" << m_socket << "is_connected=" << isConnected() 
    << "family=" << m_family << "type=" << m_type << "protocol=" << m_protocol ;
    if(m_localAddr){
        os << "local=" << m_localAddr->toString();
    }
    if(m_remoteAddr){
        os << "remote=" << m_remoteAddr->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead(){
    return IOManager::GetThis()->cancelEvent(m_socket, IOManager::READ);
}

bool Socket::cancelWrite(){
    return IOManager::GetThis()->cancelEvent(m_socket, IOManager::WRITE);
}
bool Socket::cancelAccept(){
    return IOManager::GetThis()->cancelEvent(m_socket, IOManager::READ);
}
bool Socket::cancelAll(){
    return IOManager::GetThis()->cancelAll(m_socket);
}


std::ostream& operator<<(std::ostream& os, Socket::ptr sock){
    return sock->dump(os);
}


namespace {

struct _SSLInit{
    _SSLInit(){
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }
};

static _SSLInit s_init;
}


SSLSocket::ptr SSLSocket::CreateTCP(cxk::Address::ptr address){
    SSLSocket::ptr sock(new SSLSocket(address->getFamily(), TCP, 0));
    return sock;
}



SSLSocket::ptr SSLSocket::CreateTCPSocket(){
    SSLSocket::ptr sock(new SSLSocket(IPv4, TCP, 0));
    return sock;
}



SSLSocket::ptr SSLSocket::CreateTCPSocket6(){
    SSLSocket::ptr sock(new SSLSocket(IPv6, TCP, 0));
    return sock;
}



SSLSocket::SSLSocket(int family, int type, int protocol):Socket(family, type, protocol){

}


Socket::ptr SSLSocket::accept(){
    SSLSocket::ptr sock(new SSLSocket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_socket, nullptr, nullptr);
    if(newsock == -1){
        CXK_LOG_ERROR(g_logger) << "accept failed: " << strerror(errno);
        return nullptr;
    }

    sock->m_ctx = m_ctx;
    if(sock->init(newsock)){
        return sock;
    }
    return nullptr;
}   


 bool SSLSocket::bind(const Address::ptr addr) {
    return Socket::bind(addr);
 }


bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms){
    bool v = Socket::connect(addr, timeout_ms);
    if(v){
        // 创建SSL上下文
        m_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
        // 创建SSL对象
        m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
        // 绑定socket到SSL对象上
        SSL_set_fd(m_ssl.get(), m_socket);
        // 进行SSL握手连接
        v = (SSL_connect(m_ssl.get()) == 1);
    }
    return v;
}



bool SSLSocket::listen(int backlog){
    return Socket::listen(backlog);
}


bool SSLSocket::close(){
    return Socket::close();
}


int SSLSocket::send(const void* buffer, size_t length, int flags) {
    if(m_ssl){
        return SSL_write(m_ssl.get(), buffer, length);
    }
    return -1;
}


int SSLSocket::send(const iovec* buffers, size_t length, int flags){
    if(!m_ssl){
        return -1;
    }
    int total = 0;
    for(int i = 0; i < length; ++i){
        int tmp = SSL_write(m_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);
        if(tmp <= 0){
            return tmp;
        }
        total += tmp;

        if(tmp != (int)buffers[i].iov_len){
            break;
        }
    }
    return total;
}



int SSLSocket::sendto(const void* buffer, size_t length, const Address::ptr to, int flags){
    CXK_ASSERT(false);
    return -1;
}



int SSLSocket::sendto(const iovec* buffers, size_t length, const Address::ptr to, int flags){
    CXK_ASSERT(false);
    return -1;
}


int SSLSocket::recv(void* buffer, size_t length, int flags){
    CXK_LOG_DEBUG(g_logger) << "SSLSocket::recv";
    if(m_ssl){
        return SSL_read(m_ssl.get(), buffer, length);
    }
    return -1;
}


int SSLSocket::recv(iovec* buffers, size_t length, int flags) {
    if(!m_ssl){
        return -1;
    }
    int total = 0;
    for(size_t i = 0; i < length; ++i){
        int tmp = SSL_read(m_ssl.get(), buffers[i].iov_base, buffers[i].iov_len);
        if(tmp <= 0){
            return tmp;
        }
        total += tmp;
        if(tmp != (int)buffers[i].iov_len){
            break;
        }
    }
    return total;
}


int SSLSocket::recvfrom(void* buffer, size_t length, Address::ptr from, int flags){
    CXK_ASSERT(false);
    return -1;
}


int SSLSocket::recvfrom(iovec* buffers, size_t length, Address::ptr from, int flags){
    CXK_ASSERT(false);
    return -1;
}



bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file){
    m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);

    // 加载证书链
    if(SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1){
        CXK_LOG_ERROR(g_logger) << "SSL_CTX_use_certificate_chain_file failed: " << cert_file;
        return false;
    }

    // 加载私钥文件
    if(SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1){
        CXK_LOG_ERROR(g_logger) << "SSL_CTX_use_PrivateKey_file failed: " << key_file;
        return false;
    }

    // 检查撕咬与证书匹配
    if(SSL_CTX_check_private_key(m_ctx.get()) != 1){
        CXK_LOG_ERROR(g_logger) << "SSL_CTX_check_private_key failed";
        return false;
    }
    return true;
}


std::ostream& SSLSocket::dump(std::ostream& os) const{
    os << "[SSLSocket=" << m_socket
        << " is_connected=" << m_isConnected
        << " family=" << m_family
        << " type=" << m_type
        << " protocol=" << m_protocol;
    if(m_localAddr){
        os << " local=" << m_localAddr->toString();
    }
    if(m_remoteAddr){
        os << " remote=" << m_remoteAddr->toString();
    }
    os << "]";
    return os;
}



bool SSLSocket::init(int sock){
    bool v = Socket::init(sock);
    if(v){
        m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
        SSL_set_fd(m_ssl.get(), m_socket);

        // 尝试接受SSL连接
        v = (SSL_accept(m_ssl.get()) == 1);
    }
    return v;
}


}