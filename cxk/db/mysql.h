#pragma once
#include <mysql/mysql.h>
#include <memory>
#include <functional>
#include <map>
#include <list>
#include <cxk/Thread.h>
#include "cxk/mutex.h"

namespace cxk{

class MySQL;
class IMySQLUpdate{
public:
    using ptr = std::shared_ptr<IMySQLUpdate>;

    virtual ~IMySQLUpdate() {}
    virtual int cmd(const char* format, ...) = 0;
    virtual int cmd(const char* format, va_list ap) = 0;
    virtual int cmd(const std::string& sql) = 0;

    virtual std::shared_ptr<MySQL> getMySQL() = 0;
};


class MySQLRes{
public:
    using ptr = std::shared_ptr<MySQLRes>;
    using data_cb = std::function<bool(MYSQL_ROW row, int field_count, int row_no)>;

    MySQLRes(MYSQL_RES* res, int eno, const char* estr);

    MYSQL_RES* get() const { return m_data.get(); }

    uint64_t getRows();
    uint64_t getFields();

    int getErrno() const { return m_errno; }
    const std::string& getErrstr() const { return m_errstr; }

    bool foreach(data_cb cb);

private:
    int m_errno;
    std::string m_errstr;
    std::shared_ptr<MYSQL_RES> m_data;
};


class MySQLManager;
class MySQL : public IMySQLUpdate{
friend class MySQLManager;

public:
    using ptr = std::shared_ptr<MySQL>;

    MySQL(const std::map<std::string, std::string>& args);

    bool connect();
    bool ping();

    virtual int cmd(const char* format, ...) override;
    virtual int cmd(const char* format, va_list ap) override;
    virtual int cmd(const std::string& sql) override;

    virtual std::shared_ptr<MySQL> getMySQL() override;

    uint64_t getAffectedRows();

    MySQLRes::ptr query(const char* format, ...);
    MySQLRes::ptr query(const char* format, va_list ap);
    MySQLRes::ptr query(const std::string& sql);

    const char* cmd();

    bool use(const std::string& dbname);

    const char* getError();

    uint64_t getInsertID();
private:
    bool isNeedCheck();

private:
    std::map<std::string, std::string> m_params;
    std::shared_ptr<MYSQL> m_mysql;

    std::string m_cmd;
    std::string m_dbname;

    uint64_t m_lastUsedTime;
    bool m_hasError;
};



class MySQLTransaction : public IMySQLUpdate{
public:
    using ptr = std::shared_ptr<MySQLTransaction>;

    static MySQLTransaction::ptr Create(MySQL::ptr mysql, bool autoCommit);
    ~MySQLTransaction();

    bool commit();
    bool rollback();

    virtual int cmd(const char* format, ...) override;
    virtual int cmd(const char* format, va_list ap) override;
    virtual int cmd(const std::string& sql) override;

    virtual std::shared_ptr<MySQL> getMySQL() override;

    bool isAutoCommit() const { return m_autoCommit; }
    bool isFinished() const { return m_isFinished; }
    bool isError() const { return m_hasError; }    
private:
    MySQLTransaction(MySQL::ptr mysql, bool autoCommit);
private:
    MySQL::ptr m_mysql;
    bool m_autoCommit;
    bool m_isFinished;
    bool m_hasError;
};


class MySQLManager{
public:
    using MutexType = cxk::Mutex;

    MySQLManager();
    ~MySQLManager();

    MySQL::ptr get(const std::string& name);
    void registerMySQL(const std::string& name, const std::map<std::string, std::string>& params);

    void checkConnection(int sec = 30);

    uint32_t getMaxConn() const { return m_maxConn; }
    void setMaxConn(uint32_t max) { m_maxConn = max; }

    int cmd(const std::string& name, const char* format, ...);
    int cmd(const std::string& name, const char* format, va_list ap);
    int cmd(const std::string& name, const std::string& sql);

    MySQLRes::ptr query(const std::string& name, const char* format, ...);
    MySQLRes::ptr query(const std::string& name, const char* format, va_list ap);
    MySQLRes::ptr query(const std::string& name, const std::string& sql);

    MySQLTransaction::ptr openTransaction(const std::string& name, bool autoCommit);
private:
    void freeMySQL(const std::string& name, MySQL* m);

private:
    uint32_t m_maxConn;
    MutexType m_mutex;
    std::map<std::string, std::list<MySQL*>> m_conns;
    std::map<std::string, std::map<std::string, std::string>> m_dbDeines;
};



}