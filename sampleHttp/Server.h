#pragma once
// 初始化监听的套接字
int initListenFd(unsigned short port);
// 启动epoll
int epollRun(int lfd);
// 和客户端建立连接
//int acceptClient(int lfd, int epfd);
void* acceptClient(void* arg);
// 接收http请求
//int recvHttpRequest(int cfd, int epfd);
void* recvHttpRequest(void* arg);

// 解析http请求行
int parasRequestLine(const char *line, const char *inputText, int cfd)
;

// 发送文件
int sendFile(int cfd, const char *filename);
// 发送响应头(状态行+响应头)
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length, const char *filename);
const char* getFileType(const char* name);
// 发送目录
int sendDir(int cfd, const char* dirName);
int hexToDec(char c);
void decodeMsg(char* to, char* from);

// 处理文件下载请求
int handleDownload(int cfd, const char *filePath);

// 生成数据检索的HTML部分
void generateInputSection1(char *buf);

// 物种注释分析
void generateInputSection2(char *buf);

// 生成关于部分
void generateAboutSection(char *buf);

// 生成页面底部
void generateFooterSection(char *buf);

// 处理 Get 请求
int handleGetRequest(char *path, int cfd);

// 处理 Post 请求
int handlePostRequest(char *line, const char *inputText, int cfd);