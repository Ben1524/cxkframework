#include "mysql.h"
#include "mysql/mysql.h"
#include "cxk/logger.h"
#include "cxk/config.h"

namespace cxk{

static cxk::Logger::ptr g_logger = CXK_LOG_NAME("system");
static cxk::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_mysql_dbs
    = cxk::Config::Lookup("mysql.dbs", std::map<std::string, std::map<std::string, std::string> >()
            , "mysql dbs");

namespace{

    // mysql的多线程环境初始化
    struct MySQLThreadInit{
        MySQLThreadInit(){
            mysql_thread_init();
        }

        ~MySQLThreadInit(){
            mysql_thread_end();
        }
    };
}


static MYSQL* mysql_init(std::map<std::string, std::string>& params, const int& timeout){
    static thread_local MySQLThreadInit s_thread_initer;
    MYSQL* mysql = ::mysql_init(nullptr);
    if(mysql == nullptr){
        CXK_LOG_ERROR(g_logger) << "mysql_init failed, errno: " << errno << ", errstr: " << strerror(errno);
        return nullptr;
    }

    if(timeout > 0){
        mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    }

    bool close = false;
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &close);
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "UTF8");

    int port = cxk::GetParamValue(params, "port", 0);
    std::string host = cxk::GetParamValue<std::string>(params, "host");
    std::string user = cxk::GetParamValue<std::string>(params, "user");
    std::string passwd = cxk::GetParamValue<std::string>(params, "passwd");
    std::string dbname = cxk::GetParamValue<std::string>(params, "dbname");

    if(mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(),
         dbname.c_str(), port, nullptr, 0) == nullptr){
        CXK_LOG_ERROR(g_logger) << "mysql connect failed, errno: " << mysql_errno(mysql)
            << ", errstr: " << mysql_error(mysql);
        mysql_close(mysql);
        return nullptr;
    }
    return mysql;
}


static MYSQL_RES* my_mysql_query(MYSQL* mysql, const char* sql){
    if(mysql == nullptr){
        CXK_LOG_ERROR(g_logger) << "mysql is nullptr";
        return nullptr;
    }

    if(sql == nullptr){
        CXK_LOG_ERROR(g_logger) << "sql is nullptr";
        return nullptr;
    }

    if(::mysql_query(mysql, sql)){
        CXK_LOG_ERROR(g_logger) << "mysql query failed, errno: " << mysql_errno(mysql)
            << ", errstr: " << mysql_error(mysql);
        return nullptr;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if(res == nullptr){
        CXK_LOG_ERROR(g_logger) << "mysql store result failed, errno: " << mysql_errno(mysql)
            << ", errstr: " << mysql_error(mysql);
    }

    return res;
}


MySQLRes::MySQLRes(MYSQL_RES* res, int eno, const char* estr) : m_errno(eno), m_errstr(estr){
    if(res){
        m_data.reset(res, mysql_free_result);
    }
}


uint64_t MySQLRes::getRows(){
    return mysql_num_rows(m_data.get());
}


uint64_t MySQLRes::getFields(){
    return mysql_num_fields(m_data.get());
}


bool MySQLRes::foreach(data_cb cb){
    MYSQL_ROW row;
    uint64_t fields = getFields();
    int i = 0;
    while((row = mysql_fetch_row(m_data.get()))){
        if(!cb(row, fields, i++)){
            break;
        }
    }
    return true;
}


MySQL::MySQL(const std::map<std::string, std::string>& args) : m_params(args),
     m_lastUsedTime(0), m_hasError(false){
    
}



bool MySQL::connect(){
    if(m_mysql && !m_hasError){
        return true;
    }

    MYSQL* m = mysql_init(m_params, 0);
    if(!m){
        m_hasError = true;
        return false;
    }
    m_hasError = false;

    // 利用智能指针析构时自动释放mysql资源
    m_mysql.reset(m, mysql_close);
    return true;
}


bool MySQL::ping(){
    if(!m_mysql){
        return false;
    }
    if(mysql_ping(m_mysql.get())){
        m_hasError = true;
        return false;
    }
    m_hasError = false;
    return true;
}


int MySQL::cmd(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int rt = cmd(format, ap);
    va_end(ap);
    return rt;
}


int MySQL::cmd(const char* format, va_list ap){
    m_cmd = cxk::StringUtil::formatv(format, ap);
    int r = ::mysql_query(m_mysql.get(), m_cmd.c_str());
    if(r){
        CXK_LOG_ERROR(g_logger) << "cmd = " << cmd() << ", error = " << mysql_error(m_mysql.get());
        m_hasError = true;
    } else{
        m_hasError = false;
    }
    return r;
}



int MySQL::cmd(const std::string& sql) {
    m_cmd = sql;
    int r = ::mysql_query(m_mysql.get(), m_cmd.c_str());
    if(r){
        CXK_LOG_ERROR(g_logger) << "cmd = " << cmd() << ", error = " << mysql_error(m_mysql.get());
        m_hasError = true;
    } else{
        m_hasError = false;
    }
    return r;    
}


std::shared_ptr<MySQL> MySQL::getMySQL() {
    return MySQL::ptr(this, cxk::nop<MySQL>);
}




uint64_t MySQL::getAffectedRows(){
    if(!m_mysql){
        return 0;
    }
    return mysql_affected_rows(m_mysql.get());
}


MySQLRes::ptr MySQL::query(const char* format, ...){
    va_list ap;
    va_start(ap, format);
    auto rt = query(format, ap);
    va_end(ap);
    return rt;
}


MySQLRes::ptr MySQL::query(const char* format, va_list ap){
    m_cmd = cxk::StringUtil::formatv(format, ap);
    MYSQL_RES* res = my_mysql_query(m_mysql.get(), m_cmd.c_str());
    if(!res){
        m_hasError = true;
        return nullptr;
    }
    m_hasError = false;
    MySQLRes::ptr rt(new MySQLRes(res, mysql_errno(m_mysql.get()), mysql_error(m_mysql.get())));
    return rt;
}


MySQLRes::ptr MySQL::query(const std::string& sql){
    m_cmd = sql;
    MYSQL_RES* res = my_mysql_query(m_mysql.get(), m_cmd.c_str());
    if(!res){
        m_hasError = true;
        return nullptr;
    }
    m_hasError = false;
    MySQLRes::ptr rt(new MySQLRes(res, mysql_errno(m_mysql.get()), mysql_error(m_mysql.get())));
    return rt;
}



const char* MySQL::cmd(){
    return m_cmd.c_str();
}


bool MySQL::use(const std::string& dbname){
    if(!m_mysql){
        return false;
    }
    if(m_dbname == dbname){
        return true;
    }

    if(mysql_select_db(m_mysql.get(), dbname.c_str()) == 0){
        m_dbname = dbname;
        m_hasError = false;
        return true;
    } else {
        m_dbname = "";
        m_hasError = true;
        return false;
    }
}


const char* MySQL::getError(){
    if(!m_mysql){
        return "";
    }
    const char* str = mysql_error(m_mysql.get());
    if(str){
        return str;
    }
    return "";
}


uint64_t MySQL::getInsertID(){
    if(m_mysql){
        return mysql_insert_id(m_mysql.get());
    }
    return 0;
}


bool MySQL::isNeedCheck(){
    if((time(0) - m_lastUsedTime) < 5 && !m_hasError){
        return false;
    }
    return true;
}


MySQLTransaction::ptr MySQLTransaction::Create(MySQL::ptr mysql, bool auto_commit){
    MySQLTransaction::ptr rt(new MySQLTransaction(mysql, auto_commit));
    if(rt->cmd("BEGIN") == 0){
        return rt;
    }
    return nullptr;
}

MySQLTransaction::~MySQLTransaction(){
    if(m_autoCommit){
        commit();
    } else {
        rollback();
    }
}

bool MySQLTransaction::commit(){
    if(m_isFinished || m_hasError){
        return !m_hasError;
    }
    int rt = cmd("COMMIT");
    if(rt == 0){
        m_isFinished = true;
    } else m_hasError = true;

    return rt == 0;
}

bool MySQLTransaction::rollback(){
    if(m_isFinished){
        return true;
    }
    int rt = cmd("ROLLBACK");
    if(rt == 0){
        m_isFinished = true;
    } else {
        m_hasError = true;
    }
    return rt == 0;
}   

int MySQLTransaction::cmd(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    return cmd(format, ap);
}


int MySQLTransaction::cmd(const char* format, va_list ap) {
    if(m_isFinished){
        CXK_LOG_ERROR(g_logger) << "transaction is finished, can't execute cmd = " << format;
        return -1;
    }
    int rt = m_mysql->cmd(format, ap);
    if(rt){
        m_hasError = true;
    }
    return rt;
}


int MySQLTransaction::cmd(const std::string& sql) {
    if(m_isFinished){
        CXK_LOG_ERROR(g_logger) << "transaction is finished, can't execute cmd = " << sql;
        return -1;
    }
    int rt = m_mysql->cmd(sql);
    if(rt){
        m_hasError = true;
    }
    return rt;
}

std::shared_ptr<MySQL> MySQLTransaction::getMySQL(){
    return m_mysql;
}



MySQLTransaction::MySQLTransaction(MySQL::ptr mysql, bool autoCommit) : m_mysql(mysql), m_autoCommit(autoCommit){
}



MySQLManager::MySQLManager() : m_maxConn(10){
    mysql_library_init(0, nullptr, nullptr);
}


MySQLManager::~MySQLManager(){
    mysql_library_end();
}

MySQL::ptr MySQLManager::get(const std::string& name){
    MutexType::Lock lock(m_mutex);
    auto it = m_conns.find(name);
    if(it != m_conns.end()){
        if(!it->second.empty()){
            MySQL* rt = it->second.front();
            it->second.pop_front();
            lock.unlock();
            if(!rt->isNeedCheck()){
                rt->m_lastUsedTime = time(0);
                return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL, this, name, std::placeholders::_1));
            }
            if(rt->ping()){
                rt->m_lastUsedTime = time(0);
                return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL, this, name, std::placeholders::_1));
            } else if(rt->connect()){
                rt->m_lastUsedTime = time(0);
                return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL, this, name, std::placeholders::_1));
            } else {
                CXK_LOG_ERROR(g_logger) << "connect mysql error";
                return nullptr;
            }
        }
    }

    // 没有可用连接,尝试从配置文件或已注册连接中读取
    auto config = g_mysql_dbs->getValue();
    auto sit = config.find(name);
    std::map<std::string, std::string> args;
    if(sit != config.end()){
        args = sit->second;
    } else {
        // 配置文件中没有，查找是否已经注册
        sit = m_dbDeines.find(name);
        if(sit != m_dbDeines.end()){
            args = sit->second;
        } else {
            return nullptr;
        }
    }
    lock.unlock();
    MySQL* rt = new MySQL(args);
    if(rt->connect()){
        rt->m_lastUsedTime = time(0);
        return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL, this, name, std::placeholders::_1));
    } else {
        delete rt;
        return nullptr;
    }
}


void MySQLManager::registerMySQL(const std::string& name, const std::map<std::string, std::string>& params){
    MutexType::Lock lock(m_mutex);
    m_dbDeines[name] = params;
}



void MySQLManager::checkConnection(int sec){
    time_t now = time(0);
    MutexType::Lock lock(m_mutex);
    for(auto& i : m_conns){
        for(auto it = i.second.begin(); it != i.second.end();){
            if((int)(now - (*it)->m_lastUsedTime) >= sec){
                auto tmp = *it;
                it = i.second.erase(it);
                delete tmp;
            } else {
                ++it;
            }
        }
    }
}


int MySQLManager::cmd(const std::string& name, const char* format, ...){
    va_list ap;
    va_start(ap, format);
    int rt = cmd(name, format, ap);
    va_end(ap);
    return rt;
}


int MySQLManager::cmd(const std::string& name, const char* format, va_list ap){
    auto conn = get(name);
    if(!conn){
        CXK_LOG_ERROR(g_logger) << "can't get mysql connection";
        return -1;
    }
    return conn->cmd(format, ap);
}


int MySQLManager::cmd(const std::string& name, const std::string& sql){
    auto conn = get(name);
    if(!conn){
        CXK_LOG_ERROR(g_logger) << "can't get mysql connection";
        return -1;
    }
    return conn->cmd(sql);
}


MySQLRes::ptr MySQLManager::query(const std::string& name, const char* format, ...){
    va_list ap;
    va_start(ap, format);
    auto rt = query(name, format, ap);
    va_end(ap);
    return rt;
}


MySQLRes::ptr MySQLManager::query(const std::string& name, const char* format, va_list ap){
    auto conn = get(name);
    if(!conn){
        CXK_LOG_ERROR(g_logger) << "can't get mysql connection";
        return nullptr;
    }
    return conn->query(format, ap);
}



MySQLRes::ptr MySQLManager::query(const std::string& name, const std::string& sql){
    auto conn = get(name);
    if(!conn){
        CXK_LOG_ERROR(g_logger) << "can't get mysql connection";
        return nullptr;
    }
    return conn->query(sql);
}



MySQLTransaction::ptr MySQLManager::openTransaction(const std::string& name, bool autoCommit){
    auto conn = get(name);
    if(!conn){
        CXK_LOG_ERROR(g_logger) << "can't get mysql connection";
        return nullptr;
    }
    MySQLTransaction::ptr trans(MySQLTransaction::Create(conn, autoCommit));
    return trans;
}


void MySQLManager::freeMySQL(const std::string& name, MySQL* m){
    if(m->m_mysql){
        MutexType::Lock lock(m_mutex);
        if(m_conns[name].size() < m_maxConn){
            m_conns[name].push_back(m);
            return;
        }
    }
    delete m;
}


}