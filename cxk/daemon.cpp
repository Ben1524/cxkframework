#include "daemon.h"
#include <unistd.h>
#include "logger.h"
#include <sys/types.h>
#include <sys/wait.h>
#include "config.h"


namespace cxk{

static cxk::ConfigVar<uint32_t>::ptr g_daemon_restart_interval = cxk::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");
static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");


std::string ProcessInfo::toString() const{
    std::stringstream ss;
    ss << "[ProcessInfo parent_id = " << parent_id
    << ", main_id = " << main_id
    << ", restart_count = " << restart_count
    << ", parent_start_time = " << cxk::Time2Str(parent_start_time)
    << ", main_start_time = " << cxk::Time2Str(main_start_time)
    << "]";
    return ss.str();
}


static int real_start(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb){
    return main_cb(argc, argv);
}



static int real_daemon(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb){
    if(daemon(1, 0));
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    while(true){
        pid_t pid = fork();

        if(pid == 0){
            // 子进程
            ProcessInfoMgr::GetInstance()->main_id = getpid();
            ProcessInfoMgr::GetInstance()->main_start_time = time(0);
            CXK_LOG_INFO(g_logger) << "daemon start";
            return real_start(argc, argv, main_cb);
        } else if(pid < 0){
            CXK_LOG_ERROR(g_logger) << "fork error";
        } else{
            int status = 0;
            waitpid(pid, &status, 0);
            if(status){
                CXK_LOG_ERROR(g_logger) << "child crash";
            } else {
                CXK_LOG_INFO(g_logger) << "child exit";
                break;
            }
            ProcessInfoMgr::GetInstance()->restart_count++;
            sleep(g_daemon_restart_interval->getValue());
        }   
    }

    return 0;
}



int start_daemon(int argc, char** argv, std::function<int(int argc, char** argv)> main_cb, bool is_daemon){
    if(!is_daemon){
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}


}