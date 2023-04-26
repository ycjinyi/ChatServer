#ifndef _CHAT_SERVER_
#define _CHAT_SERVER_

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer {
public:
    //创建服务器
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);
    //开始运行
    void start();
private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time);
private:
    TcpServer _server;
    EventLoop* _loop;
};

#endif