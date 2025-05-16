#include "socket_stream.h"
#include "cxk/logger.h"
#include "cxk/util.h"


namespace cxk{

SocketStream::SocketStream(Socket::ptr socket, bool owner): m_socket(socket), m_owner(owner) {

}   


SocketStream::~SocketStream(){
    if(m_owner && m_socket){
        m_socket->close();
    }
}

int SocketStream::read(void* buffer, size_t length){
    if(!isConnected()) return -1;

    auto x =  m_socket->recv(buffer, length);
    // CXK_LOG_DEBUG(CXK_LOG_ROOT()) << buffer ;
    return x;
}


int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) return -1;

    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);
    int rt = m_socket->recv(&iovs[0], iovs.size());
    if(rt > 0){
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

int SocketStream::write(const void* buffer, size_t length) {
    if(!isConnected()) return -1;
    return m_socket->send(buffer, length);
}
    
    
int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) return -1;

    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    int rt = m_socket->send(&iovs[0], iovs.size());
    if(rt > 0){
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}


bool SocketStream::isConnected() const{
    return m_socket && m_socket->isConnected();
}


void SocketStream::close() {
    if(m_socket){
        m_socket->close();
    }
}


}