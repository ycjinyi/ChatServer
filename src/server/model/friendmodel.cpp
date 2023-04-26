#include "friendmodel.hpp"
#include "db.hpp"

 bool FriendModel::insert(int userid, int friendid) {
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend(userid, friendid) values (%d, %d), (%d, %d)", 
        userid, friendid, friendid, userid);
    MySQL mysql;
    if(mysql.connect() && mysql.update(sql)) return true;
    return false;
 }

 vector<User> FriendModel::query(int userid) {
    char sql[1024] = {0};
    sprintf(sql, "select id, name, state from User inner join Friend "
        "on friendid = id and userid = %d", userid);
    vector<User> ret;
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res == nullptr) return ret;
        MYSQL_ROW row;
        while((row = mysql_fetch_row(res)) != nullptr) {
            ret.emplace_back(atoi(row[0]), string(row[1]), string(""), string(row[2]));
        }
        mysql_free_result(res);
    }
    return ret;
 }