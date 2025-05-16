#include "cxk/cxk.h"
#include "cxk/rock/rock_stream.h"


static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();


cxk::RockConnection::ptr conn(new cxk::RockConnection);

void run(){
    conn->setAutoConnect(true);
    cxk::Address::ptr addr = cxk::Address::LookupAny("127.0.0.1:8061");
    if(!conn->connect(addr)){
        CXK_LOG_INFO(g_logger) << "connect failed";
    }
    conn->start();

    cxk::IOManager::GetThis()->addTimer(1000, [](){
        cxk::RockRequest::ptr req(new cxk::RockRequest);
        static uint32_t s_sn = 0;
        req->setSn(++s_sn);
        req->setCmd(100);
        req->setBody("hello world sn = " + std::to_string(s_sn));

        auto rsp = conn->request(req, 300);
        if(rsp->response){
            CXK_LOG_INFO(g_logger) << rsp->response->toString();
        } else {
            CXK_LOG_INFO(g_logger) << "request failed";
        }
    }, true);
}


int main(){
    cxk::IOManager iom(1);
    iom.schedule(run);
    return 0;
}