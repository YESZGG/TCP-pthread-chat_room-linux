#include "client.h"

#define SERVER_PORT 8888
#define SERVER_IP "192.168.1.148"

#define FILE_BUFFER_SIZE 4096
#define FILE_NAME_SIZE 256

user_friend_p head = NULL;
int chat = 0; // 聊天的标志，1为私聊，2为群聊

void receiveAccountList(user_friend_p head, int serverFd)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    while (1)
    {
        ssize_t bytesRead = recv(serverFd, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0)
        {
            break;
        }

        // 将接收到的缓冲区拆分为单独的行
        char *line = strtok(buffer, "\n");

        while (line != NULL)
        {
            // 检查当前行是否表示帐户列表的末尾
            if (strcmp(line, "end") == 0)
            {
                return; // 帐户结束列表
            }

            // 从行中提取ID并创建新节点
            char id[10];

            sscanf(line, "%s", id);

            // 将新节点插入客户端的链接列表
            insertNode(head, id, 1);

            line = strtok(NULL, "\n");
        }

        memset(buffer, 0, BUFFER_SIZE);
    }
}

int Login_client(int serverFd)
{
    char username[100];
    char password[100];

    // 发送登录请求标志
    send(serverFd, "login", strlen("login"), 0);
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));

    printf("请输入用户名：");
    scanf("%s", username);
    // 发送用户名
    size_t len = strlen(username);
    if (len > 0 && username[len - 1] == '\n')
    {
        username[len - 1] = '\0';
    }
    send(serverFd, username, strlen(username), 0);

    printf("请输入密码：");
    scanf("%s", password);
    // 发送密码
    send(serverFd, password, strlen(password), 0);

    // 接收登录结果
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(serverFd, buffer, BUFFER_SIZE, 0) <= 0)
    {
        perror("Error receiving login response");
        exit(EXIT_FAILURE);
    }

    printf("服务器响应: %s\n", buffer);
    if (strcmp(buffer, "login fail") == 0)
    {
        return 0;
    }
    return 1;
}

void handleRegistration(int serverFd)
{

    char username[100];
    char password[100];

    // 发送注册请求标志
    send(serverFd, "register", strlen("register"), 0);
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));

    printf("请输入用户名：");
    scanf("%s", username);
    // 发送用户名
    size_t len = strlen(username);
    if (len > 0 && username[len - 1] == '\n')
    {
        username[len - 1] = '\0';
    }
    send(serverFd, username, strlen(username), 0);
    printf("请输入密码：");
    scanf("%s", password);
    fflush(stdin);
    // 发送密码
    send(serverFd, password, strlen(password), 0);

    // 接收注册结果
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(serverFd, buffer, BUFFER_SIZE, 0) <= 0)
    {
        perror("Error receiving registration response");
        exit(EXIT_FAILURE);
    }

    printf("服务器响应: %s\n", buffer);
    return;
}

void *sendMessage(void *arg)
{
    threadArg_t *threadArg = (threadArg_t *)arg;
    char user[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    traverseList(head);
    if (chat == 1)
    {
        // 发送私聊标志
        send(threadArg->serverFd, "private_message", strlen("private_message"), 0);
        printf("************ 私聊 ************\n");
        while (1)
        {
            memset(user, 0, BUFFER_SIZE);
            memset(message, 0, BUFFER_SIZE); // 清空缓冲区

            printf("请输入要私聊的对象(用户名)：");
            scanf("%s", user);
            // 发送要私聊的对象的用户名
            send(threadArg->serverFd, user, strlen(user), 0);

            // 输入私聊消息
            printf("请输入私聊消息（输入'exit'退出）：");
            scanf("%s", message);
            if (strcmp(message, "exit") == 0)
            {
                // 发送退出信号
                send(threadArg->serverFd, message, strlen(message), 0);
                chat = 0;
                break;
            }

            // 发送私聊消息
            send(threadArg->serverFd, message, strlen(message), 0);
        }
    }
    else if (chat == 2)
    {
        send(threadArg->serverFd, "group_message", strlen("group_message"), 0);
        printf("************ 群聊 ************\n");
        while (1)
        {
            memset(message, 0, BUFFER_SIZE);

            fgets(message, sizeof(message), stdin);
            size_t len = strlen(message);
            if (len > 0 && message[len - 1] == '\n')
            {
                message[len - 1] = '\0';
            }
            if (strcmp(message, "exit") == 0)
            {
                chat = 0;
                break;
            }

            send(threadArg->serverFd, message, strlen(message), 0);
        }
    }
    pthread_exit(NULL);
}

void *receiveMessage(void *arg)
{
    int serverFd = *(int *)arg;
    // 接收消息
    char buffer[BUFFER_SIZE];
    if (chat == 1)
    {
        while (1)
        {
            memset(buffer, 0, BUFFER_SIZE);
            if (recv(serverFd, buffer, BUFFER_SIZE, 0) <= 0)
            {
                perror("Error receiving private message response");
                exit(EXIT_FAILURE);
            }
            printf("\n[receive_private_message]%s\n", buffer);
            if (strcmp(buffer, "exit") == 0 || chat == 0)
            {
                break;
            }
        }
    }
    else if (chat == 2)
    {
        while (1)
        {
            memset(buffer, 0, BUFFER_SIZE);
            if (recv(serverFd, buffer, BUFFER_SIZE, 0) <= 0)
            {
                perror("Error receiving private message response");
                exit(EXIT_FAILURE);
            }
            if (chat == 0)
            {
                break;
            }
            printf("[receive_group_message]%s\n", buffer);
        }
    }
    pthread_exit(NULL);
}

/*
上传文件函数 - 用于发送文件至服务器
serverFd - 与服务器相连的套接字
*/
void uploadfile(int serverFd)
{
    send(serverFd, "upload", strlen("upload"), 0);

    char path_name[FILE_NAME_SIZE];
    printf("请输入要上传的文件名：");
    scanf("%s", path_name);

    FILE *file = fopen(path_name, "r");
    if (file == NULL)
    {
        perror("文件打开失败");
        exit(1);
    }
    char *file_name = strrchr(path_name, '/');
    if (file_name != NULL)
    {
        file_name++;
        printf("[%s] 发送文件名：%s 至服务器\n", get_formatted_time(2), file_name);
    }

    // 发送文件名给服务器
    if (send(serverFd, path_name, strlen(path_name), 0) == -1)
    {
        perror("发送文件名出错");
        exit(1);
    }
    sleep(1);

    // 计算文件大小
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    if (filesize == -1)
    {
        printf("[%s] 获取源文件大小失败.\n", get_formatted_time(2));
        fclose(file);
        return;
    }

    // 重置文件光标位置
    fseek(file, 0, SEEK_SET);

    if (send(serverFd, &filesize, sizeof(long), 0) == -1)
    {
        printf("[%s] 发送文件大小失败.\n", get_formatted_time(2));
    }
    printf("[%s] 发送文件大小:%ld 完成.\n", get_formatted_time(2), filesize);
    sleep(1);

    printf("[%s] 文件打开成功，文件正在传输...\n", get_formatted_time(2));
    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = fread(buffer, 1, FILE_BUFFER_SIZE, file)) > 0)
    {
        // 发送数据块给服务器
        ssize_t bytesSent = send(serverFd, buffer, bytesRead, 0);
        if (bytesSent == -1)
        {
            perror("发送数据块出错");
            exit(1);
        }
        printf("[%s] 已发送数据块大小: %zd 字节\n", get_formatted_time(2), bytesSent);
    }
    printf("[%s] 文件传输完成\n", get_formatted_time(2));

    // 关闭文件
    fclose(file);
}

/*
下载文件函数 - 从服务器下载文件
serverFd - 与服务器相连的套接字
*/
void downloadfile(int serverFd)
{
    send(serverFd, "download", strlen("download"), 0);

    // 从服务器接收并打印文件列表
    receive_and_print_file_list(serverFd);

    // 输入文件名
    printf("\nEnter the filename you want to download: ");
    char filename[BUFFER_SIZE];
    scanf("%s", filename);

    // 将文件名发送到服务器
    send(serverFd, filename, strlen(filename), 0);

    // 从服务器接收文件大小
    long fileSize;
    if (recv(serverFd, &fileSize, sizeof(long), 0) == -1)
    {
        perror("Failed to receive file size");
        exit(1);
    }

    printf("文件大小: %ld bytes.\n", fileSize);

    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived;

    while (fileSize > 0)
    {
        // 接收文件数据
        bytesReceived = recv(serverFd, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0)
        {
            printf("[%s] 无法接收文件数据.\n", get_formatted_time(2));
            fclose(file);
            exit(1);
        }

        // 将接收到的数据写入文件中
        fwrite(buffer, 1, bytesReceived, file);

        // 更新剩余的文件大小
        fileSize -= bytesReceived;
    }
    printf("[%s] 文件接收成功\n", get_formatted_time(2));
    fclose(file);
}

// 接收服务器发送过来的文件列表
void receive_and_print_file_list(int server_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    printf("\n共享文件夹文件列表:\n");
    while ((bytes_read = recv(server_fd, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[bytes_read] = '\0';

        if (strcmp(buffer, "END_OF_LIST") == 0)
        {
            return; // 接收完整的文件列表后退出循环
        }

        printf("%s\n", buffer);
    }

    if (bytes_read == -1)
    {
        perror("Error receiving file list");
        exit(1);
    }
}

// 获取时间，传入type来获取各种时间
const char *get_formatted_time(int type)
{
    time_t current_time;
    struct tm *time_info;

    time(&current_time);
    time_info = localtime(&current_time);

    static char formatted_time[20] = {};

    if (type == 0)
    {
        snprintf(formatted_time, sizeof(formatted_time), " ");
    }
    else if (type == 1)
    {
        snprintf(formatted_time, sizeof(formatted_time), "%04d-%02d-%02d",
                 (1900 + time_info->tm_year), (1 + time_info->tm_mon), time_info->tm_mday);
        // 年  月  日
    }
    else if (type == 2)
    {
        snprintf(formatted_time, sizeof(formatted_time), "%04d-%02d-%02d %02d:%02d:%02d",
                 (1900 + time_info->tm_year), (1 + time_info->tm_mon), time_info->tm_mday,
                 time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
        // 年  月  日  时  分  秒
    }
    // else if (type == 3)
    // {
    //     snprintf(formatted_time, sizeof(formatted_time), "%04d-%02d-%02d_%02d:%02d:%02d",
    //              (1900 + time_info->tm_year), (1 + time_info->tm_mon), time_info->tm_mday,
    //              time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
    // }
    else
    {
        // Handle invalid type here (e.g., set default value for formatted_time)
    }

    return formatted_time;
}

int main()
{
    int serverFd;
    struct sockaddr_in serverAddr;
    head = user_friend_init();
    // 创建套接字
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 连接服务器
    if (connect(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    printf("连接到服务器成功\n");

    int isLoggedIn = 0;

    while (1)
    {
        char choice[20];
        memset(choice, 0, sizeof(choice));
        if (isLoggedIn == 0)
        {
            printf("请选择操作（login-登录，register-注册）：");
            scanf("%s", choice);
        }
        else if (isLoggedIn == 1)
        {
            printf("\n请选择操作（private-私聊，group-群聊，getonline-获取在线列表,\nsendfile-上传文件,downfile-下载文件,logout-登出）：");
            scanf("%s", choice);
        }

        if (isLoggedIn == 0 && strcmp(choice, "login") == 0)
        {
            int ret = Login_client(serverFd);
            if (!ret)
            {
                continue;
            }
            receiveAccountList(head, serverFd);
            isLoggedIn = 1;
        }
        else if (isLoggedIn == 0 && strcmp(choice, "register") == 0)
        {
            // 处理注册
            handleRegistration(serverFd);
        }
        else if (isLoggedIn == 1 && ((strcmp(choice, "private") == 0) || (strcmp(choice, "group") == 0)))
        {
            if (strcmp(choice, "private") == 0)
            {
                chat = 1;
            }
            if (strcmp(choice, "group") == 0)
            {
                chat = 2;
            }
            // 创建发送和接收线程
            pthread_t sendThread, receiveThread;
            threadArg_t threadArg;
            threadArg.serverFd = serverFd;

            pthread_create(&sendThread, NULL, sendMessage, &threadArg);
            pthread_create(&receiveThread, NULL, receiveMessage, &serverFd);

            pthread_join(sendThread, NULL); // 等待发送线程完成
            pthread_cancel(receiveThread);
            pthread_join(receiveThread, NULL); // 等待接收线程完成
        }
        else if (isLoggedIn == 1 && strcmp(choice, "getonline") == 0)
        {
            traverseList(head);
            continue;
        }
        else if (isLoggedIn == 1 && strcmp(choice, "sendfile") == 0)
        {
            uploadfile(serverFd);
        }
        else if (isLoggedIn == 1 && strcmp(choice, "downfile") == 0)
        {
            downloadfile(serverFd);
        }
        else if (isLoggedIn == 1 && strcmp(choice, "logout") == 0)
        {
            printf("您已成功登出\n");
            isLoggedIn = 0;
            // 销毁用户好友链表
            destroyList(head);
            break;
        }
        else
        {
            printf("无效选择\n");
        }
    }

    close(serverFd);
    return 0;
}
