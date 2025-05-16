#include "cxk/hook.h"
#include "cxk/cxk.h"
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();
void test_sleep(){
    cxk::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        CXK_LOG_INFO(g_logger) << "sleep 2 seconds";
    });
    iom.schedule([](){
        sleep(3);
        CXK_LOG_INFO(g_logger) << "sleep 3 seconds";
    });  

    CXK_LOG_INFO(g_logger) << "test sleep";
}


void test_sock(){
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);  // 80 is the port number
    inet_pton(AF_INET, "183.2.172.42", &addr.sin_addr.s_addr);

    CXK_LOG_INFO(CXK_LOG_ROOT()) << "connect to 183.2.172.42";
    int rt = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    CXK_LOG_INFO(CXK_LOG_ROOT()) << "connect rt = " << rt;
    if(rt){
        return;
    }


    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(fd, data, sizeof(data), 0);
    CXK_LOG_INFO(CXK_LOG_ROOT()) << "send rt = " << rt;

    if(rt <= 0) return;

    std::string buf;
    buf.resize(1024);
    
    rt = recv(fd, &buf[0], buf.size(), 0);
    CXK_LOG_INFO(CXK_LOG_ROOT()) << "recv rt = " << rt;
    CXK_LOG_INFO(CXK_LOG_ROOT()) << "recv data = \n" << buf;

    if(rt <= 0) return;
    close(fd);
}

int main(int argc, char *argv[] ){
    test_sock();
    cxk::IOManager iom;
    iom.schedule(test_sock);
    return 0;
}