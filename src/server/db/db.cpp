#include <muduo/base/Logging.h>
#include "db.hpp"

static string server = "192.168.0.100";
static string user = "root";
static string password = "LJY@10045018";
static string dbname = "chatDB";

MySQL::MySQL() {
    _conn = mysql_init(nullptr);
}
MySQL::~MySQL() {
    if (_conn != nullptr)
        mysql_close(_conn);
}
bool MySQL::connect() {
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                    password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    
    if (p != nullptr) {//C/C++使用ASCII编码，如果从mysql读取到中文字符，则会显示？
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql successed!";
        return true;
    }
    LOG_INFO << "connect mysql failed!";
    return false;
} 
bool MySQL::update(string sql) {
    if (mysql_query(_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                    << sql << " update failed!";
        return false;
    }
    return true;
}
MYSQL_RES* MySQL::query(string sql) {
    if (mysql_query(_conn, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                    << sql << " query failed!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
