#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <iostream>
using namespace std;

void resetHandler(int) {
    ChatService::getInstance()->reset();
    exit(0);
} 

int main(int argc, char** argv) {
    const char* ip = nullptr;
    uint16_t port = 0;
    if(argc == 1) {
        ip = "192.168.0.100";
        port = 6666;
    } else if(argc == 3) {
        ip = argv[1];
        port = atoi(argv[2]);
    } else {
        cout << "please input as: ./chatServer 192.168.0.100 6666 or ./chatServer";
        return 0;
    }

    //服务器通过ctrl + c退出前，先重置用户状态
    signal(SIGINT, &resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();
    
    return 0;
}