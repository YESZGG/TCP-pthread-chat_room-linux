#ifndef _CLIENT_H__
#define _CLIENT_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024 // 缓冲区大小
typedef struct
{
    char username[100];
    char password[100];
} loginRequest_t;

typedef struct
{
    char username[100];
    char password[100];
} registrationRequest_t;

typedef struct
{
    int serverFd;
} threadArg_t;

typedef struct user_friend    // 登录后使用，有用户好友位
{
    char ID[10];              // 好友账号
    int flag;                 // 标识符-->0为下线，1为在线
    struct user_friend *next; // 后驱
    struct user_friend *prev; // 前驱
} user_friend_t, *user_friend_p;

// 接收好友链表的定义
extern void receiveAccountList(user_friend_p head, int serverFd);
// 初始化头节点
extern user_friend_p user_friend_init();
// 尾插节点
extern void insertNode(user_friend_p head, char id[], int flag);
// 根据用户名删除节点
extern void deleteNode(user_friend_p head, char id[]);
// 遍历链表
extern void traverseList(user_friend_p head);
// 销毁链表
extern void destroyList(user_friend_p head);

// 登录
int Login_client(int serverFd);
// 注册
void handleRegistration(int serverFd);
// 发送消息
void *sendMessage(void *arg);
// 接收消息
void *receiveMessage(void *arg);
/*
上传文件函数 - 用于发送文件至服务器
sendpath - 要上传的文件的路径
sockfd - 与服务器相连的套接字
*/
void uploadfile(int serverFd);
/*
下载文件函数 - 从服务器下载文件
downloadpath - 要下载的文件的路径
sockfd - 与服务器相连的套接字
*/
void downloadfile(int serverFd);
// 接收服务器发送过来的文件列表
void receive_and_print_file_list(int serverFd);
// 获取时间，传入type来获取各种时间
const char *get_formatted_time(int type);
#endif