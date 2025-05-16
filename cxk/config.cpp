#include "config.h"
#include "env.h"
#include <sys/stat.h>
#include "Thread.h"

namespace cxk{
static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");

// 用于遍历一个YAML节点,并将所有成员都加入到output中
static void ListAllMember(const std::string &prefix, const YAML::Node &node,
                            std::list<std::pair<std::string, const YAML::Node>> &output){
    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_.") != std::string::npos){
        CXK_LOG_ERROR(CXK_LOG_ROOT()) << "Config invalide name: " << prefix << " : " << node;
        return;
    }

    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()){
        for (auto it = node.begin(); it != node.end(); ++it){
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

// 尝试从Data中查找name
ConfigVarBase::ptr Config::LookupBase(const std::string &name){
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetData().find(name);
    return it == GetData().end() ? nullptr : it->second;
}


void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb){
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetData();
    for(auto it = m.begin(); it != m.end(); ++it){
        cb(it->second);
    }
}



static std::map<std::string, uint64_t> s_file2modifytime;
static cxk::Mutex s_mutex;

void Config::LoadFromConfDir(const std::string& path, bool force){
    std::string absolute_path = cxk::EnvMgr::GetInstance()->getAbsolutePath(path);

    std::vector<std::string> files;
    FSUtil::ListAllFile(files, absolute_path, ".yml");

    for(auto& i : files){
        struct stat st;
        lstat(i.c_str(), &st);
        {
            cxk::Mutex::Lock lock(s_mutex);
            if(!force && s_file2modifytime[i] == st.st_mtime){
                continue;
            }
            s_file2modifytime[i] = st.st_mtime;
        }

        try{
            // CXK_LOG_INFO(g_logger) << "LoadConfig: " << i;
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            CXK_LOG_INFO(g_logger) << "LoadConfig: " << i << " ok";
        } catch(...){
            CXK_LOG_ERROR(g_logger) << "Load config file error: " << i;
        }
    }
}



void Config::LoadFromYaml(const YAML::Node &root){
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);

    for (auto &i : all_nodes){
        std::string key = i.first;
        if (key.empty()){
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);
        if (var){
            if (i.second.IsScalar()){
                var->fromString(i.second.Scalar());
            }
            else{
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

}