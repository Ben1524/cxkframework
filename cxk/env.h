#pragma once
#include "cxk/Singleton.h"
#include "cxk/Thread.h"
#include "Singleton.h"
#include <map>
#include <vector>
#include <stdlib.h>

namespace cxk{

class Env{
public:
    using RWMutexType = RWMutex;

    bool init(int argc, char** argv);

    void add(const std::string& key, const std::string& val);

    bool has(const std::string& key);

    bool del(const std::string& key);

    std::string get(const std::string& key, const std::string& val = "");

    void addHelp(const std::string& key, const std::string& desc);

    void removeHelp(const std::string& key);

    void printHelp();


    const std::string& getExe() const { return m_exe; }
    const std::string& getCwd() const { return m_cwd; }


    bool setEnv(const std::string& key, const std::string& val);
    std::string getEnv(const std::string& key, const std::string& val = "");


    std::string getAbsolutePath(const std::string& path) const;
    std::string getConfigPath();
private:
    RWMutexType m_mutex;
    std::map<std::string, std::string> m_args;
    std::vector<std::pair<std::string, std::string>> m_helps;

    std::string m_program;
    std::string m_exe;
    std::string m_cwd;      // 可执行文件所在目录
};


using EnvMgr = cxk::Singleton<Env>;


}