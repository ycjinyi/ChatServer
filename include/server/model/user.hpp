#ifndef _USER_
#define _USER_
#include <string>
using namespace std;
//匹配数据表的类
class User {
public:
    User(int _id = -1, string _name = "", string _password = "", string _state = "offline"):
    id(_id), name(_name), password(_password), state(_state) {}
public:
    void setId(int _id) noexcept { id = _id; }
    void setName(string _name) noexcept { name = _name; }
    void setPassword(string _password) noexcept { password = _password; }
    void setState(string _state) noexcept { state = _state; }
    int getId() const noexcept{ return id; }
    string getName() const noexcept { return name; }
    string getPassword() const noexcept { return password; }
    string getState() const noexcept { return state; }
private:
    int id;
    string name;
    string password;
    string state;
};
#endif