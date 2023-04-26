#include "chatservice.hpp"
#include "public.hpp"
#include "user.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
using namespace muduo;

ChatService::ChatService() {
    //注册消息id对应的handler业务回调
    _msgHandlerMap.insert({msgType::LOGIN_MSG, 
        std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({msgType::REGIST_MSG, 
        std::bind(&ChatService::regist, this, _1, _2, _3)});
    _msgHandlerMap.insert({msgType::ONE_CHAT_MSG,
        std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({msgType::ADD_FRIEND_MSG,
        std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({msgType::LOGOUT_MSG,
        std::bind(&ChatService::logout, this, _1, _2, _3)});
    //连接到redis服务器
    if(_redis.connect()) {
        //指定收到订阅消息的处理函数
        _redis.init_notify_handler(bind(&ChatService::subRedisMsg, this, _1, _2));
    }
}

ChatService* ChatService::getInstance() {
    static ChatService instance;
    return &instance;
}

void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    User user = _userModel.query(js["id"].get<int>());//查询id对应的用户信息
    json response;
    response["msgid"] = msgType::LOGIN_ACK;
    if(user.getId() == js["id"].get<int>()) {//用户存在
        if(user.getPassword() == js["password"]) {//密码正确
            if(user.getState() == "offline") {//用户不在线，可以登录
                {
                    lock_guard<mutex> lg(_connMutex);//操作共享的STL需要加锁
                    _userConnMap.insert({user.getId(), conn}); //添加到在线活跃的用户中
                }
                user.setState("online");//刷新用户状态
                _userModel.updateState(user);
                //响应-----------------------------
                response["errno"] = 0;
                response["id"] = user.getId();
                response["name"] = user.getName();
                response["errmsg"] = "login successed!";
                //读取离线消息------------------------
                vector<string> msg = _offlineMsgModel.query(user.getId());
                //不为空则清空离线消息
                if(msg.size()) {
                    _offlineMsgModel.remove(user.getId());
                    response["offlineMsg"] = msg;//添加到响应中
                } 
                //读取好友列表-------------------------
                vector<User> friends = _friendModel.query(user.getId());
                if(friends.size()) {
                    vector<string> f;
                    for(auto& it: friends) {
                        json imf;
                        imf["id"] = it.getId();
                        imf["name"] = it.getName(); 
                        imf["state"] = it.getState();
                        f.push_back(imf.dump());
                    }
                    response["friends"] = f;
                }
                //注册订阅消息
                _redis.subscribe(user.getId());
             } else { //用户已经在线
                response["errno"] = 1;
                response["errmsg"] = "user already online!";
            }
        } else { //密码错误
            response["errno"] = 1;
            response["errmsg"] = "wrong password!";
        }
    } else {//用户不存在
        response["errno"] = 1;
        response["errmsg"] = "user don't exist!";
    }
    conn->send(response.dump());
}

void ChatService::regist(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    User user;
    user.setName(js["name"]);
    user.setPassword(js["password"]);
    json response;
    response["msgid"] = msgType::REGIST_ACK;
    if(_userModel.insert(user)) {//注册成功
        response["errno"] = 0;
        response["errmsg"] = "regist successed!";
        response["id"] = user.getId();
    } else {
        response["errno"] = 1;
        response["errmsg"] = "regist failed!";
    }
    conn->send(response.dump());
}

msgHandler ChatService::getHandler(int msgid) {
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end()) {//如果没有对应的id就返回空函数
        return [=] (const TcpConnectionPtr& a, json& b, Timestamp c) {
            LOG_ERROR << "msgid: " << msgid << " can't find handler!";
        };
    }
    return it->second;
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn) {
    User user;
    {
        lock_guard<mutex> lg(_connMutex);
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) {
            if(it->second = conn) {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    if(user.getId() == -1) return;
    //取消redis消息订阅
    _redis.unsubscribe(user.getId());
    user.setState("offline");
    _userModel.updateState(user);
}

void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int to = js["to"].get<int>();
    {
        lock_guard<mutex> lg(_connMutex);
        auto it = _userConnMap.find(to);
        if(it != _userConnMap.end()) {
            //如果对象在本台服务器上且在线，就直接转发消息
            it->second->send(js.dump());
            return;
        }
    }
    //通过User表查询目标用户当前是否在线，如果在线就通过发布消息的方式发送数据
    User user = _userModel.query(to);
    if(user.getState() == "offline") {
        //此时对象不在线，需要离线记录消息到mysql中
        _offlineMsgModel.insert(to, js.dump());//数据以json格式存储
        return;
    }
    //该用户在线，但不在本台服务器上，通过发布消息到redis服务器，将消息传递到对应的chatServer服务器上
    _redis.publish(to, js.dump());    
}

bool ChatService::reset() {//重置用户的状态为offline
    return _userModel.resetState();
}

void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    _friendModel.insert(userid, friendid);
}

void ChatService::subRedisMsg(int id, string msg) {
    {
        lock_guard<mutex> lg(_connMutex);
        auto it = _userConnMap.find(id);//通过订阅的消息，寻找在本服务器上的用户
        if(it != _userConnMap.end()) {
            it->second->send(std::move(msg));
            return;
        }
    }
    _offlineMsgModel.insert(id, msg);
}

//用户下线
void ChatService::logout(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int id = js["id"].get<int>();
    //下线取消订阅
    _redis.unsubscribe(id);
    {
        lock_guard<mutex> lg(_connMutex);
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end()) {
            _userConnMap.erase(id);
        }  
    }
    User user(id, "", "", "offline");
    _userModel.updateState(user);
}
