#ifndef _USER_MODEL_
#define _USER_MODEL_
class User;
//user表的操作
class UserModel {
public:
    bool insert(User& user);
    User query(int id);
    bool updateState(User& user);
    bool resetState();
};
#endif