#include "cxk/http/ws_connection.h"
#include "cxk/iomanager.h"
#include "cxk/util.h"


void run(){
    auto rt = cxk::http::WSConnection::Create("http://127.0.0.1:8020/cxk", 1000);
    if(!rt.second){
        std::cout << rt.first->toString() << std::endl;
        return ;
    }

    auto conn = rt.second;
    while(true){
        for(int i = 0; i < 1; ++i){
            conn->sendMessage(cxk::random_string(60), cxk::http::WSFrameHead::TEXT_FRAME, false);
        }

        conn->sendMessage(cxk::random_string(65), cxk::http::WSFrameHead::TEXT_FRAME, true);
        auto msg = conn->recvMessage();
        if(!msg){
            break;
        }

        std::cout << "opcode=" << msg->getOpcode()
            << "data=" << msg->getData() << std::endl;
        sleep(10);
    }
}


int main(){
    srand(time(0));
    cxk::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
