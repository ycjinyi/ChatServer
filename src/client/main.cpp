#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include "json.hpp"
#include "public.hpp"
#include "user.hpp"
using namespace std;
using json = nlohmann::json;

//当前登录的用户信息
User nowUser;
//用于用户发送消息和接收消息之间的同步
sem_t rwsem;
//用于表明是否登录成功
atomic_bool logSuccessed;
//用于表明mainMenu是否在工作
bool mainMenuRunning;
//当前登录的用户的好友列表
vector<User> userFriends;

void ckerror(int num, const char* words);//打印错误信息并退出
int connInit(int argc, char** argv);//初始化连接
void readMsg(int fd);//接收并显示来自服务器的消息
void tryLogin(int fd);//尝试登录
void tryRegister(int fd);//尝试注册
void mainMenu(int fd);//主界面
void parseRegAck(const json& js);//处理注册消息
void parseLogAck(const json& js);//处理登录信息
void parseMsg(const json& js);//处理消息
void showCurrentUser();//显示当前用户的信息

int main(int argc, char** argv) {
    int clinetFd = connInit(argc, argv);
    sem_init(&rwsem, 0, 0);
    logSuccessed = false;
    //创建一个线程专门用于接收来自服务器的消息
    thread readFromServer(readMsg, clinetFd);
    readFromServer.detach();
    while(true) {
        int choice = 0;
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        cin >> choice;
        cin.get();//读掉剩余的回车键
        switch(choice) {
            case 1:
                //尝试登录，登录成功后进入主界面
                tryLogin(clinetFd);
                if(logSuccessed) mainMenu(clinetFd);
                break;
            case 2:
                //尝试注册
                tryRegister(clinetFd);
                break;
            case 3:
                close(clinetFd);
                sem_destroy(&rwsem);
                exit(0);
                break;
            default:
                cout << "invalid input!" << endl;
                break;
        }
    }
    return 0;
}

void ckerror(int num, const char* words) {//打印错误信息并退出
    if(num == -1) {
        cerr << words << endl;
        exit(-1);
    }
}

int connInit(int argc, char** argv) {//初始化连接
    //获取服务器主机ip地址和端口号
    const char* ip = nullptr;
    uint16_t port = 0;
    if(argc == 1) { 
        ip = "192.168.0.100";
        port = 8000;//nginx反向代理监听的端口
    } else if(argc == 3) {
        ip = argv[1];
        port = atoi(argv[2]);
    } else {
        ckerror(-1, "please int put as: ./chatClient 192.168.0.100 6666 or ./chatClient");
    }
    //创建基于TCP的clientFd;
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);
    ckerror(clientFd, "create client socket failed!");
    //设置ip地址以及端口号
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    //连接服务器
    int ret = connect(clientFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(ret == -1) close(clientFd);
    ckerror(ret, "can't connect to server!");
    return clientFd;
}

void readMsg(int fd) {//接收并显示来自服务器的消息
    while(true) {
        char buf[1024] = {0};
        int ret = recv(fd, buf, 1024, 0);//阻塞读
        ckerror(ret, "recive msg from server failed!");
        if(ret == 0) break;
        
        json js = json::parse(buf);
        int msgid = js["msgid"];
        switch(msgid) {
            case msgType::LOGIN_ACK:
                parseLogAck(js);
                sem_post(&rwsem);
                break;
            case msgType::REGIST_ACK:
                parseRegAck(js);
                sem_post(&rwsem);
                break;
            case msgType::ONE_CHAT_MSG:
                parseMsg(js);
                break;
            default:
                ckerror(-1, "msg recived from server was wrong!");
                break;
        }
    }
}

void tryLogin(int fd) {//尝试登录, 等接收到了服务器消息再返回
    int id;
    char pwd[50] = {0};
    cout << "user id:";
    cin >> id;
    cin.get();
    cout << "user password:";
    cin.getline(pwd, 50);
    
    json js;
    js["msgid"] = msgType::LOGIN_MSG;
    js["id"] = id;
    js["password"] = string(pwd);

    string loginMsg = js.dump();//loginMsg.size() = strlen(loginMsg.c_str())
    int ret = send(fd, loginMsg.c_str(), loginMsg.size() + 1, 0);
    ckerror(ret, "send loginMsg to server failed!");
    
    sem_wait(&rwsem);//阻塞在信号量上，等待服务器的响应
}

void tryRegister(int fd) { //尝试注册，等接收到了服务器消息再返回
    char name[50] = {0}, pwd[50] = {0};
    cout << "user name:";
    cin.getline(name, 50);
    cout << "user password:";
    cin.getline(pwd, 50);

    json js;
    js["msgid"] = msgType::REGIST_MSG;
    js["name"] = string(name);
    js["password"] = string(pwd);

    string registerMsg = js.dump();
    int ret = send(fd, registerMsg.c_str(), registerMsg.size() + 1, 0);
    ckerror(ret, "send registerMsg to server failed!");
    
    sem_wait(&rwsem);//阻塞在信号量上，等待服务器的响应
}

void parseRegAck(const json& js) { //处理注册消息 
    cout << js["errmsg"] << endl;
    if(js["errno"] == 1) return;
    cout << "your id: " << js["id"] << endl;
}

void parseLogAck(const json& js) { //处理登录信息
    cout << js["errmsg"] << endl;
    if(js["errno"] == 1) return;
    logSuccessed = true;//登录成功
    //登录成功，记录用户信息
    nowUser.setId(js["id"].get<int>());
    nowUser.setName(js["name"]);
    nowUser.setState(string("online"));
    //处理好友列表
    if(js.contains("friends")) {
        userFriends.clear();
        vector<string> friends = js["friends"];
        for(auto& f: friends) {
            json user = json::parse(f);
            userFriends.emplace_back(user["id"].get<int>(), 
            user["name"], "", user["state"]);
        }
    }
    //显示用户信息
    showCurrentUser();
    //处理离线消息
    if(js.contains("offlineMsg")) {
        vector<string> msg = js["offlineMsg"];
        for(auto& m: msg) {
            json imf = json::parse(m);
            parseMsg(imf);
        }
    }
}

void parseMsg(const json& js) { //处理消息
    cout <<"from: " << js["from"] << " get msg: " << js["msg"] << endl;
}

void showCurrentUser() { //显示当前用户的信息
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << nowUser.getId() << " name:" << nowUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (userFriends.size()) {
        for (User &user : userFriends) {
            cout << user.getId() << " <-> " << user.getName() << " :" << user.getState() << endl;
        }
    }
    cout << "======================================================" << endl;
}

void help(int fd = 0, string str = "");//显示帮助菜单
void chat(int fd, string str);//发送消息
void add(int fd, string str);//添加好友
void logout(int fd, string str);//退出登录

//客户端的命令列表
unordered_map<string, string> command_list {
    {"help", "format: help"},
    {"chat", "format: chat:id:msg"},
    {"add", "format: add:id"},
    {"logout", "format: logout"}
};

typedef function<void(int, string)> cmd_handler;

//客户端的回调函数映射
unordered_map<string, cmd_handler> command_handler {
    {"help", help},
    {"chat", chat},
    {"add", add},
    {"logout", logout}
};

void mainMenu(int fd) {
    help();
    char buf[1024] = {0};
    mainMenuRunning = true;
    while(mainMenuRunning) {
        cin.getline(buf, 1024);
        string cmd(buf);
        int idx = cmd.find(':');
        if(idx == -1) idx = cmd.size();
        string order(cmd.substr(0, idx));
        auto it = command_handler.find(order);
        if(it == command_handler.end()) {
            cout << "invalid order!" << endl;
            continue;
        }
        cmd_handler handler = it->second;
        string param = idx == cmd.size() ? 
            string("") : cmd.substr(idx + 1, cmd.size() - idx - 1);
        handler(fd, param);
    }
}

void help(int fd, string str) {//显示帮助菜单 
    cout << "command list: " << endl;
    for(auto& it: command_list) {
        cout << "command: " << it.first << ", " << it.second << endl;
    }
    cout << endl;
}

void chat(int fd, string str) {//发送消息 
    int idx = str.find(':');
    ckerror(idx, "the format of order: chat is wrong!");
    string id(str.substr(0, idx));
    string msg(str.substr(idx + 1, str.size() - 1 - idx));
    
    json js;
    js["msgid"] = msgType::ONE_CHAT_MSG;
    js["to"] = atoi(id.c_str());
    js["from"] = nowUser.getName();
    js["id"] = nowUser.getId();
    js["msg"] = msg;

    string chatMsg = js.dump();
    int ret = send(fd, chatMsg.c_str(), chatMsg.size() + 1, 0);
    ckerror(ret, "chat msg send to server error!");
}

void add(int fd, string str) {//添加好友 
    int id = atoi(str.c_str());
    
    json js;
    js["msgid"] = msgType::ADD_FRIEND_MSG;
    js["id"] = nowUser.getId();
    js["friendid"] = id;

    string addFriendMsg = js.dump();
    int ret = send(fd, addFriendMsg.c_str(), addFriendMsg.size() + 1, 0);
    ckerror(ret, "add friend msg send to server error!");
}

void logout(int fd, string str) {//退出登录
    json js;
    js["msgid"] = msgType::LOGOUT_MSG;
    js["id"] = nowUser.getId();
    
    string logoutMsg = js.dump();
    int ret = send(fd, logoutMsg.c_str(), logoutMsg.size() + 1, 0);
    ckerror(ret, "logout msg send to server error!");
    
    nowUser.setState("offline");
    userFriends.clear();
    mainMenuRunning = false;
}

