#include "usermodel.hpp"
#include "user.hpp"
#include "db.hpp"
#include <memory>

bool UserModel::insert(User& user) {
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values ('%s', '%s', '%s')",
        user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    MySQL mysql;
    if(mysql.connect() && mysql.update(string(sql))) {
        //这里插入数据主键自增，需要查询对应的主键再设置回User表对象中
        user.setId(mysql_insert_id(mysql.getConn()));
        return true;
    }
    return false;
}

User UserModel::query(int id) {
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id = %d", id);
    MySQL mysql;
    User user;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(string(sql));
        if(res == nullptr) return user;
        MYSQL_ROW row = mysql_fetch_row(res);
        if(row == nullptr) {
            mysql_free_result(res);
            return user;
        }   
        user.setId(atoi(row[0]));
        user.setName(string(row[1]));
        user.setPassword(string(row[2]));
        user.setState(string(row[3]));
        mysql_free_result(res);
    }
    return user;
}

bool UserModel::updateState(User& user) {
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d", 
        user.getState().c_str(), user.getId());
    MySQL mysql;
    if(mysql.connect() && mysql.update(string(sql))) return true;
    return false;
}

bool UserModel::resetState() {
    char sql[1024] = {0};
    sprintf(sql, "update User set state = 'offline' where state = 'online'");
    MySQL mysql;
    if(mysql.connect() && mysql.update(sql)) return true;
    return false;
}