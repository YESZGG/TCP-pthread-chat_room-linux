#ifndef _SERVER_H__
#define _SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>


#define BUFFER_SIZE 1024 // 缓冲区大小

typedef struct
{
    int newClientFd;               // 客户端套接字描述符
    struct sockaddr_in clientAddr; // 客户端地址
    int status;                    // 客户端登录状态（0-未登录，1-已登录）
    char clientAccount[50];        // 客户端账号
    char password[50];             // 客户端密码
} clientInfo_t;

typedef struct clientNode
{
    struct clientNode *next;
    struct clientNode *prev;
    clientInfo_t info;
} clientNode_t, *client_p;

typedef struct
{
    char username[50];
    char password[50];
} loginRequest_t;

typedef struct
{
    char username[50];
    char password[50];
} registerRequest_t;


client_p initialize();
// 在链表中添加一个新的客户端节点到链表尾部
void addNode(client_p head, int newClientFd, struct sockaddr_in clientAddr, int status, char *clientAccount, char *password);
// 通过用户名删除客户端节点
void delete_By_Username(client_p head, char *account);
// 遍历客户端链表并按顺序打印出所有的IP地址、端口号和账号
void print_Client_Info(client_p head);
// 销毁客户端链表
void destroy(client_p head);
// 处理登录请求
void handleLogin(clientNode_t *node, int client_fd);
// 处理注册请求
void handleRegister(int client_fd);
// 处理接收客户端消息
void *clientRecv(void *arg);
// 检查链表中是否已存在相同账号的节点
int checkIfAccountExists(client_p head, char *account);
// 发送给客户端在线列表信息
void sendAccountList(client_p head, int client_fd);
// 获取在线人数
int get_online_num(client_p head);
// 处理私聊请求
void handlePrivateMessage(client_p head, clientNode_t *node);
// 处理群聊请求
void handleGroupMessage(client_p head, clientNode_t *node);
// 接收文件消息
void receivefile(clientNode_t *node);
void sendfile(clientNode_t *node);
// 发送文件列表
void send_file_list(int client_fd);
const char *get_formatted_time(int type);
#endif