#ifndef _DB_
#define _DB_

#include <mysql/mysql.h>
#include <string>
using namespace std;

class MySQL {
public:
    MySQL();
    ~MySQL();
    bool connect();
    bool update(string sql);
    MYSQL_RES* query(string sql);
    inline MYSQL* getConn() { return _conn; }
private:
    MYSQL* _conn;
};
#endif