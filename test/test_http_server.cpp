#include "cxk/cxk.h"

static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();


std::string g_body = 
    R"rawliteral(
    <!DOCTYPE html>
    <html>
        <head>
            <meta charset="utf-8"/>
            <title>sylar chat room</title>
        </head>
        <body>
            <script type="text/javascript">
                var websocket = null;
                function write_msg(msg) {
                    document.getElementById("message").innerHTML += msg + "<br/>";
                }
                function login() {
                    websocket = new WebSocket("ws://192.168.1.5:8072/sylar/chat");
                    websocket.onerror = function() {
                        write_msg("onerror");
                    }
                    websocket.onopen = function() {
                        var o = {};
                        o.type = "login_request";
                        o.name = document.getElementById('tname').value;
                        websocket.send(JSON.stringify(o));
                    }
                    websocket.onmessage = function(e) {
                        var o = JSON.parse(e.data);
                        if (o.type == "user_enter") {
                            write_msg("[" + o.time + "][" + o.name + "] 加入聊天室");
                        } else if (o.type == "user_leave") {
                            write_msg("[" + o.time + "][" + o.name + "] 离开聊天室");
                        } else if (o.type == "msg") {
                            write_msg("[" + o.time + "][" + o.name + "] " + o.msg);
                        } else if (o.type == "login_response") {
                            write_msg("[" + o.result + "][" + o.msg+ "]");
                        }
                    }
                    websocket.onclose = function() {
                        write_msg("服务器断开");
                    }
                }
                function sendmsg() {
                    var o = {};
                    o.type = "send_request";
                    o.msg = document.getElementById('msg').value;
                    websocket.send(JSON.stringify(o));
                }
            </script>
            昵称:<input id="tname" type="text"/><button onclick="login()">登录</button><br/>
            聊天信息:<input id="msg" type="text"/><button onclick="sendmsg()">发送</button><br/>
            <div id="message">
            </div>
        </body>
    </html>
    )rawliteral";

void run(){
    cxk::http::HttpServer::ptr server(new cxk::http::HttpServer);
    cxk::Address::ptr addr = cxk::Address::LookupAnyIpAddr("0.0.0.0:8020");
    while(!server->bind(addr)){
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/cxk", [](cxk::http::HttpRequest::ptr req, cxk::http::HttpResponse::ptr rsp, cxk::http::HttpSession::ptr session){
        // rsp->setBody(req->toString());
        rsp->setBody(g_body);
        return 0;
    });

    sd->addServlet("/cxk/xx", [](cxk::http::HttpRequest::ptr req, cxk::http::HttpResponse::ptr rsp, cxk::http::HttpSession::ptr session){
        rsp->setBody(req->toString());
        return 0;
    });


    server->start();
}

int main(){
    cxk::IOManager iom(2);
    iom.schedule(run);

    return 0;
}