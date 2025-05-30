#include "util.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <string>
#include <execinfo.h>
#include "logger.h"
#include <cstring>
#include "fiber.h"
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

namespace cxk{

cxk::Logger::ptr g_logger2 = CXK_LOG_NAME("system");

pid_t getThreadId(){
    return syscall(SYS_gettid);
}

uint32_t getFiberId(){
    return cxk::Fiber::GetFiberId();
}


void BackTrace(std::vector<std::string> &bt, int size, int skip){
    void** array = (void**)malloc(sizeof(void*) * size);
    size_t n = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, n);
    if(strings == nullptr){
        CXK_LOG_ERROR(g_logger2) << "backtrace_symbols failed";
        return ;
    }

    for(size_t i = skip; i < n; ++i){
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}

std::string BackTraceToString(int size, int skip, const std::string& prefix){
    std::vector<std::string> bt;
    BackTrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i){
        ss << prefix + bt[i] << std::endl;
    }
    return ss.str();
}


uint64_t GetCurrentMS(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul * 1000ul + tv.tv_usec;
}


std::string Time2Str(time_t ts, const std::string& format){
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}



void FSUtil::ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix){
    if(access(path.c_str(), 0) != 0){
        return;
    }

    DIR* dir = opendir(path.c_str());
    if(dir == nullptr){
        return ;
    }

    struct dirent* dp = nullptr;
    while((dp = readdir(dir)) != nullptr){
        if(dp->d_type == DT_DIR){
            if(strcmp(".", dp->d_name) == 0 || strcmp("..", dp->d_name) == 0){
                continue;
            }
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        } else if(dp->d_type == DT_REG){
            std::string filename(dp->d_name);
            if(subfix.empty()){
                files.push_back(path + "/" + filename);
            } else {
                if(filename.size() < subfix.size()){
                    // CXK_LOG_DEBUG(g_logger2) << "file:" << filename << " subfix:" << subfix;
                    continue;
                }
                if(filename.substr(filename.length() - subfix.size()) == subfix){
                    // CXK_LOG_DEBUG(g_logger2) << "find file:" << path + "/" + filename;
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }

    closedir(dir);
}


static int __lstat(const char* file, struct stat* st = nullptr) {
    struct stat lst;
    int ret = lstat(file, &lst);
    if(st) {
        *st = lst;
    }
    return ret;
}


static int __mkdir(const char* dirname){
    if(access(dirname, F_OK) == 0){
        return 0;
    }
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}



bool FSUtil::Mkdir(const std::string& dirname){
    if(__lstat(dirname.c_str()) == 0){
        return true;
    }

    char* path = strdup(dirname.c_str());
    char* ptr = strchr(path + 1, '/');
    do{
        for(; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')){
            *ptr = '\0';
            if(__mkdir(path) != 0){
                break;
            }
        }

        if(ptr != nullptr){
            break;
        } else if(__mkdir(path) != 0){
            break;
        }
        free(path);
        return true;
    } while(0);

    free(path);
    return false;
}



bool FSUtil::IsRunningPidfile(const std::string& pidfile){
    if(__lstat(pidfile.c_str()) != 0){
        return false;
    }

    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)){
        return false;
    }

    if(line.empty()){
        return false;
    }

    pid_t pid = atoi(line.c_str());
    if(pid <= 1){
        return false;
    }

    // 发送信号0给进程，测试进程是否存在
    if(kill(pid, 0) != 0){
        return false;
    }
    return true;
}


bool FSUtil::Rm(const std::string& path){
    struct stat st;
    if(lstat(path.c_str(), &st)){
        return true;
    }

    if(!(st.st_mode & S_IFDIR)){
        return Unlink(path);
    }

    DIR* dir = opendir(path.c_str());
    if(!dir){
        return false;
    }

    bool ret = true;
    struct dirent* dp = nullptr;
    // 删除目录内的所有文件和文件夹
    while(dp = readdir(dir)){
        if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")){
            continue;
        }

        std::string dirname = path + "/" + dp->d_name;
        ret = Rm(dirname);
    }

    closedir(dir);
    // 删除目录本身
    if(::rmdir(path.c_str())){
        ret = false;
    }
    return ret;
}


bool FSUtil::Mv(const std::string& from, const std::string& to){
    if(!Rm(to)){
        return false;
    }
    return rename(from.c_str(), to.c_str()) == 0;
}


bool FSUtil::Realpath(const std::string& path, std::string& rpath){
    if(__lstat(path.c_str())){
        return false;
    }

    char* ptr = ::realpath(path.c_str(), nullptr);
    if(nullptr == ptr){
        return false;
    }
    std::string(ptr).swap(rpath);
    free(ptr);
    return true;
}




bool FSUtil::Symlink(const std::string& frm, const std::string& to){
    if(!Rm(to)){
        return false;
    }
    return ::symlink(frm.c_str(), to.c_str()) == 0;
}


bool FSUtil::Unlink(const std::string& filename, bool exist){
    if(!exist && __lstat(filename.c_str())){
        return true;
    }
    return ::unlink(filename.c_str()) == 0;
}


std::string FSUtil::Dirname(const std::string& filename){
    if(filename.empty()){
        return ".";
    }
    auto pos = filename.rfind('/');
    if(pos == 0){
        return "/";
    } else if(pos == std::string::npos){
        return ".";
    } else {
        return filename.substr(0, pos);
    }
}


std::string FSUtil::Basename(const std::string& filename){
    if(filename.empty()){
        return filename;
    }

    auto pos = filename.rfind('/');
    if(pos == std::string::npos){
        return filename;
    } else {
        return filename.substr(pos + 1);
    }
}



bool FSUtil::OpenForRead(std::ifstream& ifs, const std::string& filename, std::ios_base::openmode mode){
    ifs.open(filename.c_str(), mode);
    return ifs.is_open();
}


bool FSUtil::OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode){
    ofs.open(filename.c_str(), mode);
    if(!ofs.is_open()){
        std::string dir = Dirname(filename);
        Mkdir(dir);
        ofs.open(filename.c_str(), mode);
    }
    return ofs.is_open();
}


std::string StringUtil::format(const char* fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    auto v = formatv(fmt, ap);
    va_end(ap);
    return v;
}


std::string StringUtil::formatv(const char* fmt, va_list ap){
    char* buf = nullptr;
    auto len = vasprintf(&buf, fmt, ap);
    if(len == -1) return "";
    std::string ret(buf, len);
    free(buf);
    return ret; 
}


int8_t TypeUtil::ToChar(const std::string& str){
    if(str.empty()){
        return 0;
    }
    return *str.begin();
}

int64_t TypeUtil::Atoi(const std::string& str){
    if(str.empty()){
        return 0;
    }
    return strtoll(str.c_str(), nullptr, 10);
}


double TypeUtil::Atof(const std::string& str){
    if(str.empty()){
        return 0;
    }
    return atof(str.c_str());
}


int8_t TypeUtil::ToChar(const char* str){
    if(str == nullptr){
        return 0;
    }
    return str[0];
}


int64_t TypeUtil::Atoi(const char* str){
    if(str == nullptr){
        return 0;
    }
    return strtoll(str, nullptr, 10);
}


double TypeUtil::Atof(const char* str){
    if(str == nullptr){
        return 0;
    }
    return atof(str);
}




}