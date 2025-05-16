#include <cxk/db/mysql.h>
#include <iostream>

int main(){
    cxk::MySQLManager sm;

    std::map<std::string, std::string> params;
    params["host"] = "127.0.0.1";
    params["port"] = "3306";
    params["user"] = "root";
    params["passwd"] = "tbbsx63145@Czy";
    params["dbname"] = "test_db";
    sm.registerMySQL("test", params);

    sm.checkConnection();
    
    auto res = sm.query("test", "SELECT * FROM person;");
    res->foreach([](MYSQL_ROW row, int field_count, int row_no) -> bool{
        // 处理每一行数据
        for(int i = 0; i < field_count; ++i) {
            std::cout <<row[i] << " ";
        }
        puts("");
        // 返回 true 继续遍历，返回 false 可以提前终止遍历
        return true;
    });

    return 0;
}