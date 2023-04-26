#ifndef _PUBLIC_
#define _PUBLIC_

enum msgType {
    LOGIN_MSG = 1,//登录
    LOGIN_ACK,
    REGIST_MSG,//注册
    REGIST_ACK,
    ONE_CHAT_MSG,//单点消息
    ADD_FRIEND_MSG,//添加好友消息
    LOGOUT_MSG//下线消息
};

#endif