#include "cxk/module.h"
#include "cxk/Singleton.h"
#include <iostream>
#include "cxk/http/http_server.h"
#include "cxk/application.h"
#include "cxk/logger.h"

static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

class A{
public:
    A(){
        std::cout << "A()" << std::endl;
    }

    ~A(){
        std::cout << "~A()" << std::endl;
    }
};


class MyModule : public cxk::RockModule{
public:
    MyModule() : RockModule("hello", "1.0", "") {

    }


    bool handleRockRequest(cxk::RockRequest::ptr request, cxk::RockResponse::ptr response, cxk::RockStream::ptr stream){
        CXK_LOG_INFO(g_logger) << "handleRockRequest" << request->toString();
        response->setResult(0);
        response->setResultStr("ok");
        response->setBody("echo: " + request->toString());
        return true;
    }


    bool handleRockNotify(cxk::RockNotify::ptr notify, cxk::RockStream::ptr stream){
        CXK_LOG_INFO(g_logger) << "handleRockNotify" << notify->toString();
        return true;
    }


    bool onServerReady() override{
        std::vector<cxk::TcpServer::ptr> svrs;
        cxk::Application::GetInstance()->getServer("http", svrs);
        auto http_server = std::dynamic_pointer_cast<cxk::http::HttpServer>(svrs[0]);
        auto slt_dispatch = http_server->getServletDispatch();

        slt_dispatch->addServlet("/cxk2", [](cxk::http::HttpRequest::ptr req, cxk::http::HttpResponse::ptr rsp, cxk::http::HttpSession::ptr session){
            rsp->setBody(req->toString());
            return 0;
        });
        return true;
    }



    bool onLoad() override{
        cxk::Singleton<A>::GetInstance();
        std::cout << "-------------------------- onLoad() ---------------------------" << std::endl;
        return true;
    }

    bool onUnload() override{
        cxk::Singleton<A>::GetInstance();
        std::cout << "------------------ onUnload() ----------------------" << std::endl;
        return true;
    }
};


extern "C"{

cxk::Module* CreateModule(){
    cxk::Singleton<A>::GetInstance();
    std::cout << "===============CreateModule==================" << std::endl;
    
    return new MyModule();
}


void DestoryModule(cxk::Module* ptr){
    std::cout << "===============DestoryModule==================" << std::endl;
    delete ptr;
}

}