# 基于TCP多线程的网络聊天室

### 编译方法
采用shell 脚本方式只需运行脚本即可完成编译和运行

### 下面是具体功能实现:
**登录** 
* 客户端发送登录标志请求，服务器接收消息并处理该请求．匹配客户端发送过来的密码与存储用户帐号和密码的users.txt 文本交件中的账号密码,若符号，则服务器发送允许登录的响应，客户端才可以进行其他操作
  
**注册** 
* 容户端发送注册标志.服务器则处理该请求,接收容户端发送过来的账号和密码，并将该账号密码，通过标准IO以追加形式写入到users.txt中，写入完成后，发送给客户端注册成功消息，客户端接收到success，则返回登录界面

**私聊**
* 登录成功后，将当前链表的所有帐号发送给客户端，客户端接收后，可以在这些帐号选择，客户端想与另一个客户端实现私聊，即把想私聊的客户端的账号发送消息给服务器，服务器则进入处理该请求的代码中，之后，服务器把客户端发送过来的消息，转发给另一个客户端

**群聊**
* 登录成功后，可以进入群聊模式，客户端发送消息到服务器中，服务器把接收到的消息发送给所有客户端

**上传文件**
* 客户端把文件名发送给服务器，服务器接收后创建相同文件在当前目录下，客户端再发送文件大小，服务器接收文件大小，以防止文件数据传输不正确进行对照，之后服务器接收客户端发送过来的文件内容数据

**下载文件**
* 把服务器的目录当作是共享文件夹，可以下载到客户端
  
#### 工程文件目录chatroom_project
.
├── client
│   ├── 1.txt
│   ├── 2.sh
│   ├── 3.jpg
│   ├── client
│   ├── client.c
│   └── userinfo.c
├── image
├── include
│   ├── client.h
│   └── server.h
├── READMD.md
└── server
    ├── 1.sh
    ├── 1.txt
    ├── 3.jpg
    ├── server
    ├── server.c
    └── users.txt

4 directories, 15 files
