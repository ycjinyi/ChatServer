#ifndef _OFFLINE_MSG_MODEL_
#define _OFFLINE_MSG_MODEL_

#include <vector>
#include <string>
using namespace std;

class OfflineMsgModel {
public:
    //插入离线用户消息
    bool insert(int id, string msg);
    //删除消息
    bool remove(int id);
    //查询消息
    vector<string> query(int id);
private:
};

#endif