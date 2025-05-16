#include "cxk/cxk.h"


static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();


class EchoServer : public cxk::TcpServer{
public:
    EchoServer(int type);
    void handleClient(cxk::Socket::ptr client);
private:
    int m_type;
};


EchoServer::EchoServer(int type) : m_type(type){

}

void EchoServer::handleClient(cxk::Socket::ptr client){
    CXK_LOG_INFO(g_logger) << "handleClinet" << std::endl;

    cxk::ByteArray::ptr ba(new cxk::ByteArray());
    while(true){
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        CXK_LOG_INFO(g_logger) << "recv: " << rt << std::endl;
        if(rt == 0){
            CXK_LOG_INFO(g_logger) << "client closed" << std::endl;
            break;
        } else if(rt < 0){
            CXK_LOG_ERROR(g_logger) << "recv error" << std::endl;
            break;
        } 

        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(m_type == 1){
            CXK_LOG_INFO(g_logger) << ba->toString() << std::endl;
        } else {
            CXK_LOG_INFO(g_logger) << ba->toHexString() << std::endl;
        }


    }
}


int type = 1;

void run(){
    EchoServer::ptr es(new EchoServer(type));

    auto addr = cxk::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)){
        sleep(2);
    }

    es->start();
}

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cout << "usage: ./echo_server [type]" << std::endl;
        return -1;
    }
    
    cxk::IOManager iom(2);
    iom.schedule(run);
    return 0;
}