#include "cxk/db/sqlite3.h"
#include "cxk/logger.h"
#include "cxk/util.h"



static cxk::Logger::ptr g_logger = CXK_LOG_ROOT();


void test_batch(cxk::SQLite3::ptr db){
    auto ts = cxk::GetCurrentMS();
    int n = 1000000;
    cxk::SQLite3Transaction trans(db);
    trans.begin();
    cxk::SQLite3Stmt::ptr stmt = cxk::SQLite3Stmt::Create(db, "insert into user(name, age) values(?, ?)");
    for(int i = 0; i < n; i++){
        stmt->reset();
        stmt->bind(1, "batch_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->step();
    }

    trans.commit();
    auto ts2 = cxk::GetCurrentMS();
    CXK_LOG_INFO(g_logger) << "used: " << (ts2 - ts) / 1000.0 << "s batch insert n = " << n ;
}


int main(int argc, char* argv[]){

    const std::string dbname = "test.db";
    auto db = cxk::SQLite3::Create(dbname, cxk::SQLite3::READWRITE);
    if(!db){
        CXK_LOG_INFO(g_logger) << " dbname=" << dbname << " not exist.";
        db = cxk::SQLite3::Create(dbname, cxk::SQLite3::READWRITE | cxk::SQLite3::CREATE);
        if(!db){
            CXK_LOG_INFO(g_logger) << " create dbname=" << dbname << " failed.";
        }

#define XX(...) #__VA_ARGS__
        int rt = db->execute(
XX(create table user(
        id integer primary key autoincrement,
        name varchar(50) not null default "",
        age int not null default 0

    )));
#undef XX

        if(rt != SQLITE_OK){
            CXK_LOG_ERROR(g_logger) << "create table error " << db->getErrorCode() << " - " << db->getErrorMsg();
            return 0;
        }
    }

    for(int i = 0; i < 10; ++i){
        if(db->execute("insert into user(name, age) values(\"name_%d\", %d)", i, i) != SQLITE_OK){
            CXK_LOG_ERROR(g_logger) << "insert error " << db->getErrorCode() << " - " << db->getErrorMsg();
        }
    }

    cxk::SQLite3Stmt::ptr stmt = cxk::SQLite3Stmt::Create(db, "insert into user(name, age) values(?, ?)");
    if(!stmt){
        CXK_LOG_ERROR(g_logger) << "create stmt error " << db->getErrorCode() << " - " << db->getErrorMsg();
        return 0;
    }

    for(int i = 0; i < 10; ++i){
        stmt->bind(1, "stmt_" + std::to_string(i));
        stmt->bind(2, i);

        if(stmt->execute() != SQLITE_OK){
            CXK_LOG_ERROR(g_logger) << "execute statment error " << i << " " << db->getErrorCode() << " - " << db->getErrorMsg();
        }
        stmt->reset();
    }

    cxk::SQLite3Stmt::ptr query = cxk::SQLite3Stmt::Create(db, "select * from user");
    if(!query){
        CXK_LOG_ERROR(g_logger) << "create query"   << db->getErrorCode() << " - " << db->getErrorMsg();
        return 0;
    }

    auto ds = query->query();
    if(!ds){
        CXK_LOG_ERROR(g_logger) << "query error"  << db->getErrorCode() << " - " << db->getErrorMsg();
        return 0;
    }


    do {
        int columnCount = ds->getColumnCount();
        for(int i = 0; i < columnCount; ++i){
            switch(ds->getColumnType(i)){
                case SQLITE_INTEGER:
                    CXK_LOG_INFO(g_logger) << ds->getColumnName(i) << ": " << ds->getInt64(i);
                    break;
                case SQLITE_FLOAT:
                    CXK_LOG_INFO(g_logger) << ds->getColumnName(i) << ": " << ds->getDouble(i);
                    break;
                case SQLITE_TEXT:
                    CXK_LOG_INFO(g_logger) << ds->getColumnName(i) << ": " << ds->getTextString(i);
                    break;
                case SQLITE_BLOB:
                    // 注意：这里直接将BLOB转换为string可能不总是合适的，取决于您的需求。
                    CXK_LOG_INFO(g_logger) << ds->getColumnName(i) << ": BLOB with size " << ds->getColumnBytes(i);
                    break;
                case SQLITE_NULL:
                    CXK_LOG_INFO(g_logger) << ds->getColumnName(i) << ": NULL";
                    break;
                default:
                    CXK_LOG_INFO(g_logger) << "Unknown type at column " << i;
                    break;
            }
        }
        // 打印一个空行以区分不同的行数据
        CXK_LOG_INFO(g_logger) << "";
    } while(ds->next());

    test_batch(db);
    return 0;
}