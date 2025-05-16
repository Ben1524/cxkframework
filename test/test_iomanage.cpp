#include "cxk/cxk.h"
#include "cxk/iomanager.h"
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

cxk::Logger::ptr g_logger = CXK_LOG_ROOT();

void test_fiber(){
    CXK_LOG_INFO(g_logger) << "test fiber";
}

void test1(){
    cxk::IOManager iom(2);
    iom.schedule(&test_fiber);


    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(fd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);  // 80 is the port number
    inet_pton(AF_INET, "183.2.172.185", &addr.sin_addr);
    iom.addEvent(fd, cxk::IOManager::WRITE, [=](){
        CXK_LOG_INFO(g_logger) << "connected";
        close(fd);
    });
    connect(fd, (sockaddr*)&addr, sizeof addr);
}

cxk::Timer::ptr timer;
void test_timer(){
    cxk::IOManager iom(2);
    CXK_LOG_DEBUG(g_logger) << "test_timer";
    timer = iom.addTimer(1000, [](){
        static int i = 0;
        CXK_LOG_INFO(g_logger) << "tansor nb";
        if(++i == 10){
            timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv){
    //test1();
    test_timer();
    return 0;
}

