#ifndef _CHAT_SERVICE_
#define _CHAT_SERVICE_

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "offlinemsgmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace nlohmann;

using msgHandler = function<void (const TcpConnectionPtr&, json&, Timestamp)>;

class ChatService {//单例业务类
public:
    static ChatService* getInstance();
    //登录
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //注册
    void regist(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //单点消息
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //添加好友
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //下线
    void logout(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //获取消息对应的handler
    msgHandler getHandler(int msgid);
    //用于处理用户异常断开连接的情况
    void clientCloseException(const TcpConnectionPtr& conn);
    //重置用户登录信息
    bool reset();
private:
    ChatService();
    //当有redis订阅消息时的处理函数
    void subRedisMsg(int id, string msg);
    //存储消息id和对应的业务处理方法
    unordered_map<int, msgHandler> _msgHandlerMap;
    //存储用户id和对应的Tcp链接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    //用于加锁，保证对_userConnMap的操作互斥
    mutex _connMutex;

    //用于封装User表的数据库操作
    UserModel _userModel;
    //用于封装OfflineMassage表的数据库操作
    OfflineMsgModel _offlineMsgModel;
    //用于封装Friend表的数据库操作
    FriendModel _friendModel;
    
    //用于服务器集群之间的通信
    Redis _redis;
};
#endif