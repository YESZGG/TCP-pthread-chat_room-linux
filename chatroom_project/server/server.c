#include "server.h"

#define MAX_CLIENTS 128 // 最大客户端连接数
#define SERVER_PORT 8888
#define SERVER_IP "192.168.1.148"

#define FILE_BUFFER_SIZE 4096
#define FILE_NAME_SIZE 256

pthread_mutex_t lock;

client_p head = NULL;
// 初始化客户端链表，创建头结点
client_p initialize()
{
    head = (client_p)malloc(sizeof(clientNode_t));
    if (head == NULL)
    {
        printf("Memory allocation failed.\n");
        return (client_p)-1;
    }
    head->info.newClientFd = -1;
    head->info.status = -1;
    // 初始化账号和密码为零值
    memset(head->info.clientAccount, 0, sizeof(head->info.clientAccount));
    memset(head->info.password, 0, sizeof(head->info.password));
    head->next = head;
    head->prev = head;

    return head;
}

// 在链表中添加一个新的客户端节点到链表尾部
void addNode(client_p head, int newClientFd, struct sockaddr_in clientAddr,
             int status, char *clientAccount, char *password)
{
    client_p node = (client_p)malloc(sizeof(clientNode_t));
    if (node == NULL)
    {
        perror("Error allocating memory for new node");
        exit(EXIT_FAILURE);
    }

    node->info.newClientFd = newClientFd;
    node->info.clientAddr = clientAddr;
    node->info.status = status;
    strncpy(node->info.clientAccount, clientAccount, sizeof(node->info.clientAccount));
    strncpy(node->info.password, password, sizeof(node->info.password));
    // strncpy(node->info.clientAccount, clientAccount, sizeof(node->info.clientAccount) - 1);
    // node->info.clientAccount[sizeof(node->info.clientAccount) - 1] = '\0'; // Ensure null-termination
    // strncpy(node->info.password, password, sizeof(node->info.password) - 1);
    // node->info.password[sizeof(node->info.password) - 1] = '\0'; // Ensure null-termination

    if (head->next == head)
    {
        // Empty list, add as the first node
        node->next = head;
        node->prev = head;
        head->next = node;
        head->prev = node;
    }
    else
    {
        // Non-empty list, add at the end
        node->next = head;
        node->prev = head->prev;
        head->prev->next = node;
        head->prev = node;
    }
}

// 通过用户名删除客户端节点
void delete_By_Username(client_p head, char *account)
{
    client_p p = head->next;
    while (p != head)
    {
        if (strcmp(p->info.clientAccount, account) == 0)
        {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            free(p);
            printf("[username：%s] 删除成功\n", account);
            return;
        }
        p = p->next;
    }
    printf("非常抱歉，该用户不存在\n");
}

// 遍历客户端链表并按顺序打印出所有的IP地址、端口号和账号
void print_Client_Info(client_p head)
{
    printf("Client Info:\n");
    int count = 0;
    client_p p = head->next;

    while (p != head)
    {
        printf("[%d] IP:%s, Port:%d, Account: %s, Status: %d\n", ++count,
               inet_ntoa(p->info.clientAddr.sin_addr), ntohs(p->info.clientAddr.sin_port),
               p->info.clientAccount, p->info.status);
        p = p->next;
    }
}

// 获取在线人数
int get_online_num(client_p head)
{
    client_p p = head->next;
    int count = 0;

    while (p != head)
    {
        if (p->info.status == 1)
            count++;
        p = p->next;
    }

    return count;
}

// 销毁客户端链表
void destroy(client_p head)
{
    client_p temp = head->next;
    while (temp != head)
    {
        client_p nextNode = temp->next;
        free(temp);
        temp = nextNode;
    }
    free(head);
}

// 处理接收客户端消息的线程函数
void *clientRecv(void *arg)
{
    clientNode_t *node = (clientNode_t *)arg;
    char buffer[BUFFER_SIZE];
    // 接收客户端消息
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(node->info.newClientFd, buffer, BUFFER_SIZE, 0);
        if (strcmp(buffer, "exit") == 0)
        {
            // 客户端关闭连接
            printf("[IP:%s][PORT:%d] Client disconnected\n",
                   inet_ntoa(node->info.clientAddr.sin_addr), ntohs(node->info.clientAddr.sin_port));
            node->info.status = 0;
            delete_By_Username(head, node->info.clientAccount);
            printf("当前在线的客户端数：%d\n", get_online_num(head));
            break; // 跳出循环
        }
        if (bytesRead <= 0)
        {
            if (bytesRead == 0)
            {
                // 客户端关闭连接
                printf("[IP:%s][PORT:%d] Client disconnected\n",
                       inet_ntoa(node->info.clientAddr.sin_addr), ntohs(node->info.clientAddr.sin_port));
                node->info.status = 0;
                delete_By_Username(head, node->info.clientAccount);
            }
            else
            {
                perror("Error receiving message");
            }

            close(node->info.newClientFd);
            printf("当前在线的客户端数：%d\n", get_online_num(head));
            break; // 跳出循环
        }
        else
        {
            printf("Received message from client[%s]: %s\n", node->info.clientAccount, buffer);
            // 处理登录请求
            if (strcmp(buffer, "login") == 0)
            {
                handleLogin(node, node->info.newClientFd);
            }
            // 处理注册请求
            else if (strcmp(buffer, "register") == 0)
            {
                handleRegister(node->info.newClientFd);
            }
            // 处理私聊消息
            else if (strcmp(buffer, "private_message") == 0)
            {
                handlePrivateMessage(head, node);
            }
            // 处理群聊消息
            else if (strcmp(buffer, "group_message") == 0)
            {
                handleGroupMessage(head, node);
            }
            else if (strcmp(buffer, "upload") == 0)
            {
                receivefile(node);
            }
            else if (strcmp(buffer, "download") == 0)
            {
                sendfile(node);
            }
        }
    }
    close(node->info.newClientFd);
    pthread_exit(NULL);
}

// 处理登录请求
void handleLogin(clientNode_t *node, int client_fd)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    loginRequest_t request;
    memset(&request, 0, sizeof(loginRequest_t));
    printf("登录处理中...\n");
    // 接收用户发送过来的账号和密码
    if (recv(client_fd, request.username, sizeof(request.username), 0) <= 0)
    {
        perror("Error receiving password");
        exit(EXIT_FAILURE);
    }
    printf("[Username]:%s\n", request.username);
    if (recv(client_fd, request.password, sizeof(request.password), 0) <= 0)
    {
        perror("Error receiving password");
        exit(EXIT_FAILURE);
    }
    printf("[Password]:%s\n", request.password);

    // 打开存储用户账号密码的users.txt文件
    FILE *user_fp = fopen("users.txt", "r");
    if (user_fp == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // 检查当前链表中是否已存在相同账号的节点
    if (checkIfAccountExists(head, request.username))
    {
        sprintf(buffer, "login fail - account already logged in");
    }
    else
    {
        // 逐行读取文件，并比对用户名和密码
        char line[256];
        int isLoginSuccess = 0; // 标记是否登录成功

        while (fgets(line, sizeof(line), user_fp))
        {
            char store_Username[100];
            char store_Password[100];

            sscanf(line, "%s %s", store_Username, store_Password);

            // 匹配账号和密码
            if (strcmp(request.username, store_Username) == 0 && strcmp(request.password, store_Password) == 0)
            {
                sprintf(buffer, "login success\n");

                pthread_mutex_lock(&lock); // 上锁
                // 将账号和密码赋值给当前的客户端链表节点
                strncpy(node->info.clientAccount, request.username, sizeof(node->info.clientAccount));
                strncpy(node->info.password, request.password, sizeof(node->info.password));
                node->info.status = 1;
                addNode(head, node->info.newClientFd, node->info.clientAddr, node->info.status,
                        node->info.clientAccount, node->info.password);
                print_Client_Info(head);

                isLoginSuccess = 1;          // 登录成功标记
                pthread_mutex_unlock(&lock); // 解锁
                break;                       // 登录成功，退出循环
            }
        }

        // 若未匹配到用户，则登录失败
        if (!isLoginSuccess)
        {
            sprintf(buffer, "login fail");
        }
    }

    fclose(user_fp);

    // 发送匹配结果给客户端
    send(client_fd, buffer, strlen(buffer), 0);

    // 如果登录成功，则向客户端发送包含链表中所有账号的消息
    if (strstr(buffer, "success") != NULL)
    {
        // 向每个连接的客户端发送帐户列表
        client_p current = head->next;
        while (current != head)
        {
            if (current->info.status == 1)
            { // 检查是否在线（登录成功）
                sendAccountList(head, current->info.newClientFd);
            }
            current = current->next;
        }
    }
}

// 发送给客户端在线列表信息
void sendAccountList(client_p head, int client_fd)
{
    client_p current = head->next;
    char buffer[BUFFER_SIZE];
    // int ack = 1;
    while (current != head)
    {
        // send(client_fd, &ack, sizeof(int), 0);

        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "%s\n", current->info.clientAccount);
        usleep(100000);
        send(client_fd, buffer, strlen(buffer), 0);
        current = current->next;
    }
    usleep(100000);
    // 发送一个结束信号
    // ack = 0;
    // send(client_fd, &ack, sizeof(int), 0);
    snprintf(buffer, BUFFER_SIZE, "end\n");
    send(client_fd, buffer, strlen(buffer), 0);
}

// 检查链表中是否已存在相同账号的节点
int checkIfAccountExists(clientNode_t *head, char *account)
{
    client_p curr = head;
    do
    {
        if (strcmp(curr->info.clientAccount, account) == 0)
        {
            return 1;
        }
        curr = curr->next;
    } while (curr != head);

    return 0;
}

// 处理注册请求
void handleRegister(int client_fd)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    // 注册请求
    registerRequest_t user;
    memset(&user, 0, sizeof(registerRequest_t));
    printf("注册处理中...\n");
    // 接收用户名和密码
    if (recv(client_fd, user.username, sizeof(user.username), 0) <= 0)
    {
        perror("Error receiving username");
        exit(EXIT_FAILURE);
    }
    printf("[Username]:%s\n", user.username);
    if (recv(client_fd, user.password, sizeof(user.password), 0) <= 0)
    {
        perror("Error receiving password");
        exit(EXIT_FAILURE);
    }
    printf("[Password]:%s\n", user.password);

    // 打开存储用户账号密码的文件
    FILE *user_fp = fopen("users.txt", "r+");
    if (user_fp == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // 检查文件中是否已经存在相同的账号
    char line[256];
    while (fgets(line, sizeof(line), user_fp))
    {
        char stored_username[100];
        char stored_password[100];

        sscanf(line, "%s %s", stored_username, stored_password);

        // 检查是否存在相同的账号
        if (strcmp(user.username, stored_username) == 0)
        {
            sprintf(buffer, "register fail - same account exists");
            fclose(user_fp);

            // 发送注册结果给客户端
            if (send(client_fd, buffer, sizeof(buffer), 0) == -1)
            {
                perror("register send fail");
            }
            return; // 注册失败直接返回
        }
    }

    // 将账号和密码写入文件
    fprintf(user_fp, "%s %s\n", user.username, user.password);

    fclose(user_fp);
    // 若注册成功，则返回 register success
    if (strcmp(buffer, "register fail") != 0)
    {
        sprintf(buffer, "register success");
    }
    // 发送注册结果给客户端
    if (send(client_fd, buffer, sizeof(buffer), 0) == -1)
    {
        perror("register send fail");
    }
}

// 处理私聊消息
void handlePrivateMessage(client_p head, clientNode_t *node)
{
    clientNode_t *sender = node;
    char recipientAccount[50];

    while (1)
    {
        memset(recipientAccount, 0, sizeof(recipientAccount));
        // 接收私聊 发送者 发送过来的想私聊的 接收者的用户名
        if (recv(sender->info.newClientFd, recipientAccount, sizeof(recipientAccount), 0) <= 0)
        {
            perror("Error receiving recipient account name");
            exit(EXIT_FAILURE);
        }

        // 在链接列表中查找收件人的地址信息
        client_p recipient = head->next;
        while (recipient != head)
        {
            if (strcmp(recipient->info.clientAccount, recipientAccount) == 0)
            {
                break; // 找到收件人节点
            }
            recipient = recipient->next;
        }

        if (recipient == head)
        {
            // 列表中找不到收件人
            printf("Recipient '%s' not found\n", recipientAccount);
            continue;
        }

        char message[BUFFER_SIZE];
        memset(message, 0, BUFFER_SIZE);

        // 接收私聊消息
        if (recv(sender->info.newClientFd, message, BUFFER_SIZE, 0) <= 0)
        {
            perror("Error receiving private message");
            exit(EXIT_FAILURE);
        }
        if (strcmp(message, "exit") == 0)
        {
            break;
        }
        // 在服务器上打印私人消息
        printf("[IP:%s][PORT:%d][Private Message] %s: %s\n", inet_ntoa(sender->info.clientAddr.sin_addr), ntohs(sender->info.clientAddr.sin_port), sender->info.clientAccount, message);

        // 将私人消息转发给收件人
        send(recipient->info.newClientFd, message, strlen(message), 0);
    }
}

// 处理群聊消息
void handleGroupMessage(client_p head, clientNode_t *node)
{
    clientNode_t *sender = node;
    char message[100];
    int ret = 0;
    while (1)
    {
        memset(message, 0, sizeof(message));
        ret = recv(sender->info.newClientFd, message, sizeof(message), 0);
        if (ret == 0)
        {
            printf("[Account:%s] 客户端已下线\n", sender->info.clientAccount);
        }
        if (strcmp(message, "exit") == 0)
        {
            break;
        }

        // 遍历链表中的所有客户端，发送群聊消息给每个客户端（除了发送者自身）
        for (client_p p = head->next; p != head; p = p->next)
        {
            if (p->info.status == 1) // 检查客户端在线状态
            {
                int bytesSent = send(p->info.newClientFd, message, strlen(message), 0);
                if (bytesSent <= 0)
                {
                    perror("Error sending group message");
                    // 关闭连接
                    close(p->info.newClientFd);
                }
            }
        }
    }
    return;
}

// 接收文件
void receivefile(clientNode_t *node)
{
    char buffer[FILE_BUFFER_SIZE];
    int ret = 0;
    // 接收文件名
    ssize_t filenameLength = recv(node->info.newClientFd, buffer, FILE_BUFFER_SIZE - 1, 0);
    if (filenameLength == -1)
    {
        perror("接收文件名出错");
        exit(1);
    }

    buffer[filenameLength] = '\0';
    // 将文件名解析，读取最后一个' / '之后的字符串
    FILE *file;
    char *filenamePtr = strrchr(buffer, '/');
    if (filenamePtr != NULL)
    {
        // 提取文件名并创建新文件
        filenamePtr++; // 移动指针到文件名起始位置（去除 '/'）
        file = fopen(filenamePtr, "w");
    }
    else
    {
        // 在当前目录下创建文件
        file = fopen(buffer, "w");
    }

    if (file == NULL)
    {
        perror("文件打开失败");
        exit(1);
    }

    // 接受客户端发送的文件大小
    long file_size = 0; // file_size占8个字节
    ret = recv(node->info.newClientFd, &file_size, sizeof(long), 0);
    printf("[%s] 接收文件大小:%ld 成功.\n", get_formatted_time(2), file_size);

    ssize_t bytesRead;
    size_t totalBytesRead = 0;

    while ((bytesRead = recv(node->info.newClientFd, buffer, FILE_BUFFER_SIZE, 0)) > 0)
    {
        // 写入数据块到文件
        if (fwrite(buffer, 1, bytesRead, file) < bytesRead)
        {
            perror("写入数据块出错");
            exit(1);
        }
        totalBytesRead += bytesRead;
        printf("[%s] 已接收数据块大小: %zd 字节\n", get_formatted_time(2), bytesRead);

        // 如果已接收到所有文件数据，则中断循环
        if (totalBytesRead >= file_size)
            break;
    }

    printf("文件接收完成\n");
    fclose(file);
}

// 发送文件
void sendfile(clientNode_t *node)
{
    send_file_list(node->info.newClientFd);
    
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = 0;
    // 接收来自客户端的文件名
    bytesRead = recv(node->info.newClientFd, buffer, BUFFER_SIZE, 0);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        const char *filename = buffer;

        printf("[%s] Received filename: %s\n", get_formatted_time(2), filename);

        // 以只读打开文件
        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
            perror("Error opening file");
            exit(1);
        }

        // 获取文件大小
        fseek(file, 0, SEEK_END);
        long filesize = ftell(file);
        if (filesize == -1)
        {
            printf("[%s] 无法获取源文件大小.\n", get_formatted_time(2));
            fclose(file);
            return;
        }
        fseek(file, 0, SEEK_SET);

        // 发送文件大小给客户端
        if (send(node->info.newClientFd, &filesize, sizeof(long), 0) == -1)
        {
            printf("[%s] 无法发送文件大小.\n", get_formatted_time(2));
            fclose(file);
            return;
        }
        printf("[%s] 发送的文件大小: %ld bytes.\n", get_formatted_time(2), filesize);

        // 发送数据给客户端
        char fileData[BUFFER_SIZE];
        ssize_t bytesRead;
        while ((bytesRead = fread(fileData, 1, BUFFER_SIZE, file)) > 0)
        {
            if (send(node->info.newClientFd, fileData, bytesRead, 0) == -1)
            {
                printf("[%s] 发送文件数据失败.\n", get_formatted_time(2));
                break;
            }
        }
        printf("[%s] 文件传输完成\n", get_formatted_time(2));

        // 关闭文件
        fclose(file);
    }
}
// 发送文件列表
void send_file_list(int client_fd)
{
    DIR *dir;
    struct dirent *entry;

    // 打开当前目录
    dir = opendir(".");
    if (dir == NULL)
    {
        perror("Error opening directory");
        exit(1);
    }

    // 读取目录中的每个文件
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG && entry->d_name[0] != '.') // Regular file and not a hidden file
        {
            char *filename = entry->d_name;
            // 发送给客户端
            send(client_fd, filename, strlen(filename), 0);
            send(client_fd, "\n", 1, 0);
        }
    }
    // 发送结束标志给客户端
    send(client_fd, "END_OF_LIST", strlen("END_OF_LIST"), 0);
    // 关闭目录指针
    closedir(dir);
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
    // 变量定义
    int serverFd, newClientFd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    head = initialize();
    pthread_mutex_init(&lock, NULL);

    // 创建服务器套接字
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 设置端口复用
    int optval = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 设置服务器地址
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);

    // 绑定服务器套接字到指定端口
    if (bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }
    printf("[IP:%s][PORT:%d] 服务器绑定成功\n", SERVER_IP, SERVER_PORT);
    // 监听连接请求
    if (listen(serverFd, MAX_CLIENTS) == -1)
    {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }
    printf("Listening port %d ...\n", SERVER_PORT);

    while (1)
    {
        // 接收连接请求
        newClientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &addrLen);
        if (newClientFd < 0)
        {
            perror("Accept failed");
            continue;
        }
        printf("[IP:%s][PORT:%d] 新的客户端上线\n",
               inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        clientNode_t *newNode = malloc(sizeof(clientNode_t));
        if (newNode == NULL)
        {
            perror("Memory allocation failed");
            close(newClientFd);
            continue;
        }

        newNode->info.newClientFd = newClientFd;
        memcpy(&newNode->info.clientAddr, &clientAddr, addrLen);
        pthread_t thread;
        if (pthread_create(&thread, NULL, clientRecv, (void *)newNode) != 0)
        {
            perror("Thread creation failed");
            free(newNode);
            close(newClientFd);
            continue;
        }
        sleep(1);
        // 分离线程，使其自动终止并清理
        pthread_detach(thread);
    }
    // 销毁链表
    destroy(head);
    // 关闭服务器套接字
    close(serverFd);

    return 0;
}
