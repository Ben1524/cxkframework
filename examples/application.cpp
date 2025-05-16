#include <cxk/application.h>



void func(cxk::http::HttpServer::ptr server){
    // auto sd = server->getServletDispatch();

    // sd->addServlet("/cxk", [](cxk::http::HttpRequest::ptr req, cxk::http::HttpResponse::ptr rsp, cxk::http::HttpSession::ptr session){
    //     rsp->setBody(req->toString());
    //     return 0;
    // });
}


int main(int argc, char* argv[]){
    cxk::Application app;
    if(app.init(argc, argv)){
        // app.server_cb = func;
        app.run();
        CXK_LOG_DEBUG(CXK_LOG_ROOT()) << "main thread id: " << std::this_thread::get_id();
    }
    return 0;
}
