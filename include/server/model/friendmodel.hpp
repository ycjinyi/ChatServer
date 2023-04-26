#ifndef _FRIEND_MODEL_
#define _FRIEND_MODEL_

#include "user.hpp"
#include <vector>
using namespace std;

class FriendModel {
public:
    //添加好友关系
    bool insert(int userid, int friendid);
    //返回用户的好友列表
    vector<User> query(int userid);
private:
};

#endif