#pragma once

#include "stream.h"
#include "Singleton.h"
#include "mutex.h"
#include <map>
#include "cxk/rock/rock_stream.h"
#include <unordered_map>


namespace cxk{

class Module{
public:
    using ptr = std::shared_ptr<Module>;

    enum Type{
        MODULE = 0,
        ROCK = 1,
    };

    Module(const std::string& name, const std::string& version,
         const std::string& filename, uint32_t type = MODULE);

    virtual  ~Module() {}

    virtual void onBeforeArgsParse(int argc, char** argv);
    virtual void onAfterArgsParse(int argc, char** argv);

    virtual bool onLoad();
    virtual bool onUnload();

    virtual bool onConnect(cxk::Stream::ptr stream);
    virtual bool onDisconnect(cxk::Stream::ptr stream);


    virtual bool onServerReady();
    virtual bool onServerUp();

    virtual bool handleRequest(cxk::Message::ptr req, cxk::Message::ptr rsp, cxk::Stream::ptr stream);
    virtual bool handleNotify(cxk::Message::ptr notify, cxk::Stream::ptr stream);

    virtual std::string statusString();

    const std::string& getName() const { return m_name; }
    const std::string& getVersion() const { return m_version;}
    const std::string& getFilename() const { return m_filename;}
    const std::string& getId() const { return m_id;}

    void setFilename(const std::string& v) { m_filename = v;}
    
    uint32_t getType() const { return m_type; }
protected:
    std::string m_name;
    std::string m_version;
    std::string m_filename;
    std::string m_id;
    uint32_t m_type;
};


class RockModule : public Module{
public:
    using ptr = std::shared_ptr<RockModule>;

    RockModule(const std::string& name, const std::string& version, const std::string& filename);

    virtual bool handleRockRequest(cxk::RockRequest::ptr request, cxk::RockResponse::ptr response, cxk::RockStream::ptr stream) = 0;

    virtual bool handleRockNotify(cxk::RockNotify::ptr request, cxk::RockStream::ptr stream) = 0;

    virtual bool handleRequest(cxk::Message::ptr req, cxk::Message::ptr rsp, cxk::Stream::ptr stream);
    virtual bool handleNotify(cxk::Message::ptr notify, cxk::Stream::ptr stream);
};




class ModuleManager{
public:
    using RWMutexType = RWMutex;

    ModuleManager();

    void add(Module::ptr m);
    void del(const std::string& name);
    void delAll();

    void init();

    Module::ptr get(const std::string& name);

    void onConnect(Stream::ptr stream);
    void onDisconnect(Stream::ptr stream);

    void listAll(std::vector<Module::ptr>& ms);

    void listByType(uint32_t type, std::vector<Module::ptr>& ms);
    void foreach(uint32_t type, std::function<void(Module::ptr)> cb);

private:
    void initModule(const std::string& path);

private:
    RWMutexType m_mutex;
    std::unordered_map<std::string, Module::ptr> m_modules;
    std::unordered_map<uint32_t, std::unordered_map<std::string, Module::ptr>> m_type2Modules;
};


using ModuleMgr = cxk::Singleton<ModuleManager>;



}