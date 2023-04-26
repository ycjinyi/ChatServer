#include <functional>
#include <string>
#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
using namespace std;
using namespace placeholders;
using namespace nlohmann;

ChatServer::ChatServer(EventLoop* loop, const InetAddress& listenAddr, 
const string& nameArg): _server(loop, listenAddr, nameArg), _loop(loop) {
    //指定用户断开和连接的回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    //指定收发数据的回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    //设置线程数目
    _server.setThreadNum(4);
}

void ChatServer::start() { 
    _server.start(); 
}

void ChatServer::onConnection(const TcpConnectionPtr& conn) {
    if(!conn->connected()) {
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) {
    string buf = buffer->retrieveAllAsString();
    //发送数据前先通过json序列化,收到后再通过json反序列化得到数据
    json js = json::parse(buf);
    //通过js中对应的类型调用对应的handler,要将网络模块和业务模块解耦
    auto msgHandler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js, time);
} 