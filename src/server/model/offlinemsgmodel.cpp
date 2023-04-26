#include "offlinemsgmodel.hpp"
#include "db.hpp"

bool OfflineMsgModel::insert(int id, string msg) {
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage(userid, message) values (%d, '%s')", 
        id, msg.c_str());
    MySQL mysql;
    if(mysql.connect() && mysql.update(string(sql))) return true;
    return false;
}

bool OfflineMsgModel::remove(int id) {
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid = %d", id);
    MySQL mysql;
    if(mysql.connect() && mysql.update(string(sql))) return true;
    return false;
}

vector<string> OfflineMsgModel::query(int id) {
    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid = %d", id);
    vector<string> msg;
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res == nullptr) return msg;
        MYSQL_ROW row;
        //userid不是主键，因此不唯一，可能有多条消息，需要不断拿取
        while((row = mysql_fetch_row(res)) != nullptr) {
            msg.push_back(row[0]);
        }
        mysql_free_result(res);
    }
    return msg;
}