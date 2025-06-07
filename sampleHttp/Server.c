#include"Server.h"
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<stdio.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include<sys/stat.h>
#include<assert.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/sendfile.h>
#include<dirent.h>
#include<ctype.h>
#include<pthread.h>

#define MAX_PATH_LEN 1024

struct FdInfo
{
    int fd;
    int epfd;
    pthread_t tid;
};

int initListenFd(unsigned short port)
{
    // 1. 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        return -1;
    }
    // 2. 设置端口复用
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
        return -1;
    }
    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
    if (ret == -1)
    {
        perror("bind");
        return -1;
    }
    // 4. 设置监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        return -1;
    }
    // 返回fd
    return lfd;
}

int epollRun(int lfd)
{
    // 1. 创建epoll实例
    int epfd = epoll_create(1);
    if (epfd == -1)
    {
        perror("epoll_create");
        return -1;
    }
    // 2. lfd 上树
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl");
        return -1;
    }
    // 3. 检测
    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(struct epoll_event);
    while (1)
    {
        int num = epoll_wait(epfd, evs, size, -1);
        for (int i = 0; i < num; ++i)
        {
            struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
            int fd = evs[i].data.fd;
            info->epfd = epfd;
            info->fd = fd;
            if (fd == lfd)
            {
                // 建立新连接 accept
                // acceptClient(lfd, epfd);
                pthread_create(&info->tid, NULL, acceptClient, info);
            }
            else
            {
                // 主要是接收对端的数据
                // recvHttpRequest(fd, epfd);
                pthread_create(&info->tid, NULL, recvHttpRequest, info);
            }
        }
    }
    return 0;
}

//int acceptClient(int lfd, int epfd)
void* acceptClient(void* arg)
{
    struct FdInfo* info = (struct FdInfo*)arg;
    // 1. 建立连接
    int cfd = accept(info->fd, NULL, NULL);
    if (cfd == -1)
    {
        perror("accept");
        return NULL;
    }
    // 2. 设置非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    // 3. cfd添加到epoll中
    struct epoll_event ev;
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl");
        return NULL;
    }
    printf("acceptcliet threadId: %ld\n", info->tid);
    free(info);
    return NULL;
}

// 从请求体中提取 inputText 参数（假设请求体格式是 x-www-form-urlencoded）
char* extractInputTextFromBody(const char* body) {
    char *start = strstr(body, "inputText=");
    if (start) {
        start += strlen("inputText=");  // 移动到值的开始
        char *end = strchr(start, '&');
        if (!end) {
            end = strchr(start, '\0');  // 请求体末尾
        }
        if (end) {
            size_t len = end - start;
            char *inputText = (char *)malloc(len + 1);
            strncpy(inputText, start, len);
            inputText[len] = '\0';
            return inputText;
        }
    }
    return NULL;
}

// 获取请求体中的文件内容
void extractContentsFromBody(const char *requestBody, int contentLength) {
    // 获取文件名
    char *fileContent = strstr(requestBody, "filename=\"");
    if (!fileContent) {
        printf("No file content found in the request body.\n");
        return;
    }

    fileContent += 10; // 跳过 "filename=\""
    char *filenameEnd = strchr(fileContent, '"');
    if (!filenameEnd) {
        printf("Invalid filename format.\n");
        return;
    }

    *filenameEnd = '\0'; // 将文件名字符串终止
    char *fileData = strstr(filenameEnd + 1, "\r\n\r\n");
    if (!fileData) {
        printf("No file data found in the request body.\n");
        return;
    }

    fileData += 4; // 跳过 "\r\n\r\n"

    // 计算文件数据的长度
    int fileDataLen = contentLength - (fileData - requestBody);

    // 构造文件路径
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "/home/users-data/liukai/liukai/workspace/fungi_class/dataset/test_dataset/%s", fileContent);

    // 保存文件内容
    int fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }

    ssize_t written = 0;
    while (written < fileDataLen) {
        ssize_t bytesWritten = write(fd, fileData + written, fileDataLen - written);
        if (bytesWritten == -1) {
            perror("write");
            close(fd);
            return;
        }
        written += bytesWritten;
    }

    close(fd);
    printf("File saved to: %s\n", filePath);
}


// 接收客户端的请求数据
void* recvHttpRequest(void *arg) {
    struct FdInfo *info = (struct FdInfo *)arg;
    int len = 0, total = 0;
    char temp[1024] = {0};  // 防止发生数据覆盖，先用一个缓冲区接收数据
    char buf[8192] = {0};   // 用于存放接收的数据(客户端发过来的)
    
    // 接收数据
    while ((len = recv(info->fd, temp, sizeof(temp), 0)) > 0) {
        if (total + len < sizeof(buf)) {  // 防止数据溢出
            memcpy(buf + total, temp, len);
        }
        total += len;
    }

    // 判断是否接收完毕
    if (len == -1 && errno == EAGAIN) {  // 如果接收数据遇到 EAGAIN 错误
        // 解析请求头，查找 Content-Length
        char *contentLengthHeader = strstr(buf, "Content-Length: ");
        int contentLength = 0;
        if (contentLengthHeader) {
            sscanf(contentLengthHeader, "Content-Length: %d", &contentLength);
        }

        // 打印请求头
        printf("Full HTTP Request:\n%s\n", buf);

        // 请求体的起始位置就是头部信息的最后（根据 Content-Length）
        char *requestBody = strstr(buf, "\r\n\r\n");
        if (requestBody) {
            requestBody += 4; // 跳过 \r\n\r\n
            
            // 确保请求体长度是有效的
            if (strlen(requestBody) >= contentLength) {
                // 解析请求行
                char *pt = strstr(buf, "\r\n");
                int reqlen = pt - buf;  // 请求行的长度
                buf[reqlen] = '\0';  // 请求行的结束符
                char *requestLine = buf;  // 请求行
                
                // 打印请求行和请求体
                printf("Request line: %s\n", requestLine);
                printf("Request body: %s\n", requestBody);

                // 调用 parasRequestLine，将请求行和 requestBody 一起传递
                parasRequestLine(requestLine, requestBody, info->fd);
            }
        }
    }
    else if (len == 0) {  // 客户端关闭连接
        printf("Client closed connection.\n");
        close(info->fd);
    }
    else {
        perror("recv");
    }

    printf("recvMsg threadID: %ld\n", info->tid);
    free(info);
    return NULL;
}


int parasRequestLine(const char *line, const char *requestBody, int cfd)
{
    // 解析请求行   GET /index.html HTTP/1.1
    char method[12] = {0};  // 请求方法
    char path[1024] = {0};  // 请求路径
    sscanf(line, "%[^ ] %[^ ]", method, path);
    // char *inputText = extractInputTextFromBody(requestBody);  // 从请求体中提取 inputText 参数
    // 判断请求方式是 GET 还是 POST
    if (strcasecmp(method, "GET") == 0)
    {
        printf("1 GET request received path: %s\n", path);
        // printf("1 GET request received inputText: %s\n", inputText);
        return handleGetRequest(path, cfd);  // 处理 GET 请求
    }
    else if (strcasecmp(method, "POST") == 0)
    {
        printf("1 POST request received path: %s\n", path);
        // printf("1 POST request received inputText: %s\n", inputText);
        return handlePostRequest(path, requestBody, cfd);  // 处理 POST 请求
    }
    else
    {
        return 0;  // 其他方法不支持
    }
}

// 处理 GET 请求的函数
int handleGetRequest(char *path, int cfd)
{
    // 解析 path 和其他逻辑
    decodeMsg(path, path);  // 假设你有 URL 解码函数 decodeMsg

    // 处理文件下载请求（判断 URL 中是否有 download 参数）
    if (strncmp(path, "/download?", 10) == 0)
    {
        // 提取下载的文件路径
        const char *filePath = strchr(path, '=') + 1;
        printf("Download file: %s\n", filePath);
        if (filePath)
        {
            // 调用 handleDownload 来处理文件下载
            return handleDownload(cfd, filePath);
        }
    }

    // 处理客户端请求的静态资源（目录或文件）
    char* file = NULL;
    if (strcmp(path, "/") == 0)  // 请求的是根目录
    {
        file = "./";  // 默认目录
    }
    else
    {
        file = path + 1;  // 去掉请求路径前的“/”
    }

    // 判断请求的资源是目录还是文件
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)  // 文件不存在
    {
        // 文件不存在，回复 404 错误
        sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1, NULL);
        sendFile(cfd, "404.html");
        return 0;
    }

    // 判断是目录还是文件
    if (S_ISDIR(st.st_mode))  // 请求的是目录
    {
        // 发送目录内容
        sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1, NULL);
        printf("file1: %s\n", file);
        sendDir(cfd, file);
    }
    else  // 请求的是文件
    {
        // 发送文件内容
        sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size, NULL);
        sendFile(cfd, file);
    }

    return 0;
}

// 处理 POST 请求的函数
int handlePostRequest(char *path, const char *requestBody, int cfd)
{
    // 解析 path 和其他逻辑
    decodeMsg(path, path);  // 假设你有 URL 解码函数 decodeMsg

    char * inputText = extractInputTextFromBody(requestBody);  // 从请求体中提取 inputText 参数

    // 打印 POST 请求的 path
    printf("POST request received. Path: %s, inputText: %s\n", path, inputText);

    char buf[4096] = {0};   // 用于存放目录信息
    // 解析 path 和 inputText
    // 在这里可以执行与 POST 请求相关的处理
    printf("POST request received. Path: %s, inputText: %s\n", path, inputText);

    // 处理客户端请求的静态资源（目录或文件）
    
    // 根据 path 和 inputText 进行处理
    if (strncmp(path, "/search", 7) == 0)
    {
        // 示例：执行搜索操作
        // 这里可以做具体的搜索操作，并返回相应结果
        printf("Performing search for inputText: %s\n", inputText);

        // 要传递的参数
        const char *species_name = inputText;

        // 构造命令字符串，传递参数给 Python 脚本
        char command[1024];
        sprintf(command, "python3 /home/users-data/liukai/liukai/workspace/fungi_class/src/retrieval.py %s", species_name);

        // 执行 Python 脚本
        int ret = system(command);
        if (ret == -1) {
            perror("检索失败");  // 如果 system() 调用失败
        }
        else {
            printf("检索完成\n");
        }

    }
    else if (strncmp(path, "/upload", 7) == 0) 
    {
        // 处理上传请求
        // 假设请求体为文件上传内容，提取文件内容并保存
        printf("Handling file upload request.\n");

        // 提取文件内容并保存
        char *contentLengthHeader = strstr(requestBody, "Content-Length: ");
        int contentLength = 0;
        if (contentLengthHeader) {
            sscanf(contentLengthHeader, "Content-Length: %d", &contentLength);
        }
        extractContentsFromBody(requestBody, contentLength);   // 调用提取文件内容并保存函数
        // 调用提取文件内容并保存函数

        if (chdir("/home/users-data/liukai/liukai/workspace/fungi_class") != 0) {
            perror("无法更改工作目录");
        }

        // 执行脚本
        char command2[1024];
        sprintf(command2, "conda run -n deeplearning python3 /home/users-data/liukai/liukai/workspace/fungi_class/src/main.py");
        int ret2 = system(command2);
        if (ret2 == -1) {
            perror("注释失败");  // 如果 system() 调用失败
        }
        else {
            printf("注释完成\n");
        }
    }
    else
    {
        // 其他 POST 请求的处理
        printf("Unknown POST request path: %s\n", path);
    }

    return 0;
}


const char* getFileType(const char* name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";	// 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

int sendDir(int cfd,const char *dirname)
{
    // 发送目录信息
    char buf[10240] = {0};   // 用于存放目录信息
    char note[1024] = {0};  // 用于存放数据检索的内容
    strcpy(note, "请输入物种名称");
   
    // HTML 头部，包含CSS样式
    sprintf(buf, 
        "<html><head><title>真菌物种注释系统</title>"
        "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css\">"
        "<link href=\"https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap\" rel=\"stylesheet\">"
        "<style>"
        "body { font-family: 'Roboto', sans-serif; margin: 0; padding: 0; background: linear-gradient(-45deg, #4CAF50, #007bff, #4CAF50, #007bff); background-size: 400%% 400%%; animation: gradientBG 20s ease infinite; }"
        "@keyframes gradientBG { "
        "0%% { background-position: 0%% 50%%; }"
        "50%% { background-position: 100%% 50%%; }"
        "100%% { background-position: 0%% 50%%; }"
        "}"
        ".container { width: 95%%; max-width: 1400px; margin: 0 auto; padding: 20px; background-color: rgba(255, 255, 255, 0.9); border-radius: 15px; box-shadow: 0 6px 12px rgba(0, 0, 0, 0.15); margin-top: 20px; animation: fadeIn 1s ease-in-out; }"
        "@keyframes fadeIn { "
        "from { opacity: 0; }"
        "to { opacity: 1; }"
        "}"
        ".header-h1 { text-align: center; font-size: 2.8rem; font-weight: bold; color: white; line-height: 120px; background: linear-gradient(135deg, #4CAF50, #007bff); margin: 0; padding: 20px 0; text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.2); box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); border-bottom: 2px solid #4CAF50; }"
        ".header-h1 i { margin-right: 10px; color: white; }"
        ".content-h1 { text-align: left; margin-left: 40px; color: #333; padding: 10px 0; font-size: 1.8rem; font-weight: bold; border-bottom: 2px solid #4CAF50; display: inline-block; margin-bottom: 10px; }"
        ".card { background-color: white; border-radius: 10px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); padding: 20px; margin-bottom: 20px; }"
        "table { width: 95%%; margin: 0 auto; border-collapse: separate; border-spacing: 0; margin-top: 20px; border-radius: 15px; overflow: hidden; box-shadow: 0 6px 12px rgba(0, 0, 0, 0.15); background-color: white; }"
        "th, td { padding: 12px 15px; text-align: left; border-bottom: 1px solid #ddd; transition: background-color 0.2s ease; }"
        "th { background-color: #4CAF50; color: white; font-weight: bold; }"
        "tr:hover { background-color: #f5f5f5; }"
        "tr:active { background-color: #d1e7dd; }"
        "tr:nth-child(even) { background-color: #f8f9fa; }"
        ".download-btn { background-color: #007bff; color: white; padding: 8px 12px; text-decoration: none; border-radius: 5px; transition: background-color 0.3s ease, transform 0.2s ease, box-shadow 0.3s ease; display: inline-flex; align-items: center; gap: 5px; }"
        ".download-btn:hover { background-color: #0056b3; transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); }"
        ".download-btn i { font-size: 14px; }"
        "@media (max-width: 768px) { "
        ".header-h1 { font-size: 2rem; line-height: 80px; }"
        ".content-h1 { margin-left: 20px; font-size: 1.4rem; }"
        "table { width: 100%%; }"
        "th, td { padding: 10px; }"
        "}"
        "</style>"
        "</head><body>"
        "<div class=\"container\">"
        "<h1 class=\"header-h1\"><i class=\"fas fa-seedling\"></i>真菌物种注释分析</h1>"
        "<div class=\"card\">"
        "<h1 class=\"content-h1\">目录：%s</h1>"
        "<table><tr><th>文件名</th><th>大小 (字节)</th><th>操作</th></tr>", dirname);


    struct dirent **namelist;
    int num = scandir(dirname, &namelist, NULL, alphasort);

    for (int i = 0; i < num; i++)
    {
        // 取出文件名
        char *name = namelist[i]->d_name;
        // 判断文件类型
        struct stat st;
        // 拼接文件路径
        char path[1024] = {0};
        sprintf(path, "%s/%s", dirname, name);
        // 获取文件属性
        stat(path, &st);

        if (S_ISDIR(st.st_mode))
        {
            // 目录，使用绿色链接并加粗
            sprintf(buf + strlen(buf), 
                "<tr><td><a href=\"%s/\" style=\"color: #28a745;\"><b>%s/</b></a></td><td>%ld</td></tr>", name, name, st.st_size);
        }
        else
        {
            // 文件，使用蓝色链接
            sprintf(buf + strlen(buf), 
                "<tr><td><a href=\"%s\" style=\"color: #007bff;\">%s</a></td><td>%ld</td>"
                "<td><a href=\"/download?file=%s\" class=\"download-btn\">下载</a></td></tr>", name, name, st.st_size, path);
        }  

        // 发送当前目录条目
        send(cfd, buf, strlen(buf), 0);
        // 清空buf，准备下一个条目的发送
        memset(buf, 0, sizeof(buf));

        // 释放namelist[i]
        free(namelist[i]);
    }
    // 生成关于区域
    generateAboutSection(buf);
    send(cfd, buf, strlen(buf), 0);
    // 生成数据检索的区域
    generateInputSection1(buf);
    send(cfd, buf, strlen(buf), 0);
    // 生成物种注释的区域
    generateInputSection2(buf);
    send(cfd, buf, strlen(buf), 0);
    // 生成页脚区域
    generateFooterSection(buf);
    send(cfd, buf, strlen(buf), 0);
    
    // html尾部标签拼接
    sprintf(buf, "</table></body></html>");
    // 发送尾部HTML内容
    send(cfd, buf, strlen(buf), 0);
    // 释放namelist
    free(namelist);
    
    return 0;
}


int sendFile(int cfd, const char *filename)
{
    // 发送文件
    // 1. 打开文件
    int fd = open(filename, O_RDONLY);
    // assert(fd > 0);  // 断言文件打开成功     // 老是中断程序，禁用了

    // 2. 读取文件内容

#if 0
    while (1)
    {
        char buf[1024];
        int len = read(fd, buf, sizeof(buf));
        if(len > 0){
            send(cfd, buf, len, 0);
            usleep(10); // 防止发送过快
        }
        else if(len == 0){
            // 读完了
            break;
        }
        else{
            perror("read");
            return -1;
        }
    }
#else
    off_t offset = 0;
    // 提供了一个更简单的发送文件的方法
    // 计算文件大小
    int size = lseek(fd, 0, SEEK_END);  // 文件指针移动到文件末尾

    lseek(fd, 0, SEEK_SET);  // 文件指针移动到文件开头
    while(offset < size)
    {
        int len = sendfile(cfd, fd, &offset, size-offset);  // 响应格式中的数据部分
        if(len == -1 && errno == EAGAIN)
        {
            perror("sendfile");
        }
    }

#endif
    // 3. 关闭文件
    close(fd);
    return 0;
}

int sendHeadMsg(int cfd, int status, const char *desp, const char *type, int length, const char *filename)
{
    // 发送响应头（状态行、响应头）
    // 状态行
    char buf[4096] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, desp);

    // 响应头
    sprintf(buf + strlen(buf), "Content-Type: %s\r\n", type);
    sprintf(buf + strlen(buf), "Content-Length: %d\r\n", length);

    // 如果文件下载，加入 Content-Disposition 头
    if (filename)
    {
        sprintf(buf + strlen(buf), "Content-Disposition: attachment; filename=\"%s\"\r\n", filename);
    }

    // 空行
    sprintf(buf + strlen(buf), "\r\n");

    // 发送响应头
    send(cfd, buf, strlen(buf), 0);
    return 0;
}

// 将字符转换为整形数
int hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
    for (; *from != '\0'; ++to, ++from)
    {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝, 赋值
            *to = *from;
        }

    }
    *to = '\0';
}


int setNonBlocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);  // 获取当前的文件状态标志
    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        return -1;
    }
    
    // 设置为非阻塞模式
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1)
    {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}

int handleDownload(int cfd, const char *filePath)
{
    // 设置 cfd 为非阻塞模式
    if (setNonBlocking(cfd) == -1)
    {
        return -1;  // 如果设置失败，返回错误
    }

    // 打开文件
    FILE *file = fopen(filePath, "rb");
    if (file == NULL)
    {
        const char *notFound = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
        send(cfd, notFound, strlen(notFound), 0);
        return -1;
    }

    const char *filename = strrchr(filePath, '/');
    if (filename == NULL)
    {
        filename = filePath;  // 如果没有路径部分，则直接使用文件名
    }
    else
    {
        filename++;  // 去掉路径部分，获取文件名
    }

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 发送 HTTP 响应头
    sendHeadMsg(cfd, 200, "OK", "application/octet-stream", fileSize, filename);

    char fileBuffer[1024];
    size_t bytesRead;
    ssize_t bytesSent, totalBytesSent = 0;
    while ((bytesRead = fread(fileBuffer, 1, sizeof(fileBuffer), file)) > 0)
    {
        while (totalBytesSent < bytesRead)
        {
            bytesSent = send(cfd, fileBuffer + totalBytesSent, bytesRead - totalBytesSent, 0);
            if (bytesSent == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // 如果套接字缓冲区满，则稍后重试
                    usleep(1000);  // 可以适当休眠一下，减少 CPU 占用
                    continue;
                }
                perror("send error");
                fclose(file);
                return -1;
            }
            totalBytesSent += bytesSent;
        }
        totalBytesSent = 0;  // 重置已发送字节数
    }

    fclose(file);
    return 0;
}


// 数据检索
void generateInputSection1(char *buf) {
    sprintf(buf,
        // 添加标题部分
        "<tr><td colspan=\"3\">"
            "<div style=\"background: linear-gradient(135deg, #f9f9f9, #e0e0e0); "
            "padding: 30px; border-radius: 15px; box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15); "
            "margin-top: 30px; margin-bottom: 20px; animation: fadeIn 05s ease-out; "
            "transition: transform 0.3s ease, box-shadow 0.3s ease;\" "
            "onmouseover=\"this.style.transform='translateY(-8px)'; this.style.boxShadow='0 10px 30px rgba(0, 0, 0, 0.2)';\" "
            "onmouseout=\"this.style.transform='translateY(0)'; this.style.boxShadow='0 8px 24px rgba(0, 0, 0, 0.15)';\">"
                "<h1 style=\"margin-left: -8px; "
                "margin-bottom: 20px; "
                "font-family: 'Arial', sans-serif; "
                "color: #333; "
                "border-bottom: 3px solid #007bff; "
                "display: inline-block; "
                "padding-bottom: 10px; "
                "font-size: 24px;\">真菌数据检索</h1>"

                // 输入区域部分
                "<form action=\"/search\" method=\"post\" class=\"input-section\" "
                "style=\"display: flex; justify-content: space-between; align-items: flex-start; margin-top: 20px;\" "
                "onsubmit=\"handleSubmit(event)\">"

                    // 左侧多行文本框
                    "<div style=\"flex: 1; margin-left: -8px; margin-right: 20px;\">"
                        "<textarea id=\"inputText\" name=\"inputText\" placeholder=\"真菌物种名称...\" rows=\"6\" "
                        "style=\"width: 100%%; padding: 12px; border-radius: 10px; border: 1px solid #ccc; "
                        "font-size: 14px; resize: none; background-color: #f9f9f9; "
                        "transition: border-color 0.3s ease-in-out;\" "
                        "onfocus=\"this.style.borderColor='#007bff';\" "
                        "onblur=\"this.style.borderColor='#ccc';\"></textarea>"
                    "</div>"

                    // 右侧上传文件和下载按钮
                    "<div style=\"flex: 0.3; text-align: center;\">"
                        "<div class=\"button-container\" style=\"text-align: center;\">"
                            // 检索按钮
                            "<button type=\"submit\" class=\"btn-upload1\" "
                            "style=\"padding: 14px 28px; background-color: #007bff; color: white; font-size: 16px; "
                            "border: none; border-radius: 12px; cursor: pointer; width: 100%%; "
                            "box-shadow: 0 6px 8px rgba(0, 123, 255, 0.2); "
                            "transition: background-color 0.3s ease, box-shadow 0.3s ease, transform 0.3s ease;\">"
                            "检索"
                            "</button>"
                            "<br><br>"
                            // 下载按钮
                            "<button type=\"button\" "
                            "onclick=\"window.location.href='/download?file=/home/users-data/liukai/liukai/workspace/fungi_class/output/Genus_species.csv'\" "
                            "class=\"btn-download1\" "
                            "style=\"padding: 14px 28px; background-color: #28a745; color: white; font-size: 16px; "
                            "border: none; border-radius: 12px; cursor: pointer; width: 100%%; "
                            "box-shadow: 0 6px 8px rgba(40, 167, 69, 0.2); "
                            "transition: background-color 0.3s ease, box-shadow 0.3s ease, transform 0.3s ease;\">"
                            "下载检索结果"
                            "</button>"
                        "</div>"
                    "</div>"
                "</form>"

                // 自定义模态框的 HTML 结构（第一个弹窗）
                "<div id=\"customAlert1\" style=\"display: none; position: fixed; top: 0; left: 0; width: 100%%; "
                "height: 100%%; background-color: rgba(0, 0, 0, 0.5); justify-content: center; align-items: center; "
                "z-index: 1000;\">"
                    "<div style=\"background-color: white; padding: 30px; border-radius: 10px; "
                    "box-shadow: 0 6px 10px rgba(0, 0, 0, 0.2); text-align: center; width: 400px; "
                    "transition: transform 0.3s ease;\">"
                        "<p style=\"font-size: 18px; margin-bottom: 20px; color: #333;\">正在检索...稍等几秒</p>"
                    "</div>"
                "</div>"

                // 自定义模态框的 HTML 结构（第二个弹窗）
                "<div id=\"customAlert2\" style=\"display: none; position: fixed; top: 0; left: 0; width: 100%%; "
                "height: 100%%; background-color: rgba(0, 0, 0, 0.5); justify-content: center; align-items: center; "
                "z-index: 1000;\">"
                    "<div style=\"background-color: white; padding: 30px; border-radius: 10px; "
                    "box-shadow: 0 6px 10px rgba(0, 0, 0, 0.2); text-align: center; width: 400px; "
                    "transition: transform 0.3s ease;\">"
                        "<p style=\"font-size: 18px; margin-bottom: 20px; color: #333;\">检索完成，可以下载结果</p>"
                        "<button onclick=\"closeCustomAlert2()\" style=\"padding: 10px 20px; background-color: #28a745; "
                        "color: white; border: none; border-radius: 5px; cursor: pointer;\">确定</button>"
                    "</div>"
                "</div>"

                // 自定义模态框的 HTML 结构（输入为空时的提示弹窗）
                "<div id=\"customAlert3\" style=\"display: none; position: fixed; top: 0; left: 0; width: 100%%; "
                "height: 100%%; background-color: rgba(0, 0, 0, 0.5); justify-content: center; align-items: center; "
                "z-index: 1000;\">"
                    "<div style=\"background-color: white; padding: 30px; border-radius: 10px; "
                    "box-shadow: 0 6px 10px rgba(0, 0, 0, 0.2); text-align: center; width: 400px; "
                    "transition: transform 0.3s ease;\">"
                        "<p style=\"font-size: 18px; margin-bottom: 20px; color: #333;\">请输入物种名称</p>"
                        "<button onclick=\"closeCustomAlert3()\" style=\"padding: 10px 20px; background-color: #dc3545; "
                        "color: white; border: none; border-radius: 5px; cursor: pointer;\">确定</button>"
                    "</div>"
                "</div>"

                // 添加 JavaScript 代码
                "<script>"
                    "function handleSubmit(event) {"
                        "event.preventDefault();"
                        "var inputText = document.getElementById('inputText').value.trim();"
                        "if (!inputText) {" // 如果输入框为空
                            "showCustomAlert3();" // 显示第三个弹窗
                            "return;"
                        "}"
                        "setTimeout(function() {" // 等待 1 秒后显示第一个弹窗
                            "showCustomAlert1();"
                            "setTimeout(function() {" // 5 秒后关闭第一个弹窗并提交表单
                                "closeCustomAlert1();"
                                "event.target.submit();"
                                "setTimeout(function() {" // 1.5 秒后显示第二个弹窗
                                    "showCustomAlert2();"
                                "}, 1500);"
                            "}, 5000);"
                        "}, 1000);" // 1 秒延迟
                    "}"
                    "function showCustomAlert1() {"
                        "document.getElementById('customAlert1').style.display = 'flex';"
                    "}"
                    "function closeCustomAlert1() {"
                        "document.getElementById('customAlert1').style.display = 'none';"
                    "}"
                    "function showCustomAlert2() {"
                        "document.getElementById('customAlert2').style.display = 'flex';"
                    "}"
                    "function closeCustomAlert2() {"
                        "document.getElementById('customAlert2').style.display = 'none';"
                    "}"
                    "function showCustomAlert3() {"
                        "document.getElementById('customAlert3').style.display = 'flex';"
                    "}"
                    "function closeCustomAlert3() {"
                        "document.getElementById('customAlert3').style.display = 'none';"
                    "}"
                "</script>"

                // 按钮的悬停效果样式
                "<style>"
                    ".btn-upload1:hover {"
                    "background-color: #0056b3;"
                    "box-shadow: 0 8px 12px rgba(0, 123, 255, 0.4);"
                    "transform: scale(1.05);"
                    "}"
                    ".btn-download1:hover {"
                    "background-color: #218838;"
                    "box-shadow: 0 8px 12px rgba(40, 167, 69, 0.4);"
                    "transform: scale(1.05);"
                    "}"
                "</style>"
            "</div>"
        "</td></tr>");
}

// 物种注释分析
void generateInputSection2(char *buf) {
    sprintf(buf,
        // 添加标题部分
        "<tr><td colspan=\"3\">"
            "<div style=\"background: linear-gradient(135deg, #f9f9f9, #e0e0e0); "
            "padding: 30px; border-radius: 15px; box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15); "
            "margin-top: 30px; margin-bottom: 20px; animation: fadeIn 05s ease-out; "
            "transition: transform 0.3s ease, box-shadow 0.3s ease;\" "
            "onmouseover=\"this.style.transform='translateY(-8px)'; this.style.boxShadow='0 10px 30px rgba(0, 0, 0, 0.2)';\" "
            "onmouseout=\"this.style.transform='translateY(0)'; this.style.boxShadow='0 8px 24px rgba(0, 0, 0, 0.15)';\">"
                "<h1 style=\"margin-left: -8px; "
                "margin-bottom: 20px; "
                "font-family: 'Arial', sans-serif; "
                "color: #333; "
                "border-bottom: 3px solid #007bff; "
                "display: inline-block; "
                "padding-bottom: 10px; "
                "font-size: 24px;\">真菌物种注释</h1>"

                // 输入区域部分
                "<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\" class=\"input-section\" "
                "style=\"display: flex; "
                "justify-content: space-between; "
                "align-items: flex-start; "
                "margin-top: 20px;\">"

                    // 左侧多行文本框（仅保留样式，不参与逻辑）
                    "<div style=\"flex: 1; "
                    "margin-left: -8px; "
                    "margin-right: 20px;\">"
                        "<textarea id=\"fileUpload_inputText\" name=\"inputText\" placeholder=\"真菌ITS序列数据...\" rows=\"6\" "
                        "style=\"width: 100%%; "
                        "padding: 12px; "
                        "border-radius: 10px; "
                        "border: 1px solid #ccc; "
                        "font-size: 14px; resize: none; "
                        "background-color: #f9f9f9; "
                        "transition: border-color 0.3s ease-in-out;\" "
                        "onfocus=\"this.style.borderColor='#007bff';\" "
                        "onblur=\"this.style.borderColor='#ccc';\"></textarea>"
                    "</div>"

                    // 右侧上传文件和下载按钮
                    "<div style=\"flex: 0.3; text-align: center;\">"
                        "<div class=\"button-container\" style=\"text-align: center;\">"
                            // 上传文件按钮
                            "<button type=\"button\" onclick=\"fileUpload_showFileInput()\" class=\"btn-upload1\" "
                            "style=\"padding: 14px 28px; background-color: #007bff; color: white; font-size: 16px; border: none; "
                            "border-radius: 12px; cursor: pointer; width: 100%%; box-shadow: 0 6px 8px rgba(0, 123, 255, 0.2); "
                            "transition: background-color 0.3s ease, box-shadow 0.3s ease, transform 0.3s ease;\">"
                            "上传文件"
                            "</button>"
                            "<input type=\"file\" id=\"fileUpload_fileInput\" name=\"file\" style=\"display: none;\" "
                            "onchange=\"fileUpload_uploadFile(this.files)\">"
                            "<br><br>"
                            // 下载按钮
                            "<button type=\"button\" onclick=\"window.location.href='/download?file=/home/users-data/liukai/liukai/workspace/fungi_class/output/predictions_with_labels.csv'\" "
                            "class=\"btn-download1\" style=\"padding: 14px 28px; background-color: #28a745; color: white; "
                            "font-size: 16px; border: none; border-radius: 12px; cursor: pointer; width: 100%%; "
                            "box-shadow: 0 6px 8px rgba(40, 167, 69, 0.2); transition: background-color 0.3s ease, box-shadow "
                            "0.3s ease, transform 0.3s ease;\">下载注释结果</button>"
                        "</div>"
                    "</div>"
                "</form>"

                // 自定义模态框的 HTML 结构（第一个弹窗）
                "<div id=\"fileUpload_customAlert1\" style=\"display: none; position: fixed; top: 0; left: 0; width: 100%%; "
                "height: 100%%; background-color: rgba(0, 0, 0, 0.5); justify-content: center; align-items: center; "
                "z-index: 1000;\">"
                    "<div style=\"background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 6px 10px rgba(0, "
                    "0, 0, 0.2); text-align: center; width: 400px; transition: transform 0.3s ease;\">"
                        "<p id=\"fileUpload_uploadStatus\" style=\"font-size: 18px; margin-bottom: 20px; color: #333;\">正在上传文件...</p>"
                    "</div>"
                "</div>"

                // 自定义模态框的 HTML 结构（第二个弹窗）
                "<div id=\"fileUpload_customAlert2\" style=\"display: none; position: fixed; top: 0; left: 0; width: 100%%; "
                "height: 100%%; background-color: rgba(0, 0, 0, 0.5); justify-content: center; align-items: center; "
                "z-index: 1000;\">"
                    "<div style=\"background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 6px 10px rgba(0, "
                    "0, 0, 0.2); text-align: center; width: 400px; transition: transform 0.3s ease;\">"
                        "<p style=\"font-size: 18px; margin-bottom: 20px; color: #333;\">物种注释完成，可以下载分析结果</p>"
                        "<button onclick=\"fileUpload_closeCustomAlert2()\" style=\"padding: 10px 20px; background-color: #28a745; "
                        "color: white; border: none; border-radius: 5px; cursor: pointer;\">确定</button>"
                    "</div>"
                "</div>"

                // 添加 JavaScript 代码
                "<script>"
                    "function fileUpload_showFileInput() {"
                    "document.getElementById('fileUpload_fileInput').click();"
                    "}"
                    "function fileUpload_uploadFile(files) {"
                    "if (files.length === 0) {"
                    "alert('请选择一个文件！');"
                    "return;"
                    "}"
                    "var fileName = files[0].name;"
                    "document.getElementById('fileUpload_uploadStatus').innerText = '已上传：' + fileName + '，正在分析数据，请稍等。';"
                    "setTimeout(function() {" // 等待 0.5 秒后显示第一个弹窗
                    "fileUpload_showCustomAlert1();"
                    "setTimeout(function() {"
                    "fileUpload_closeCustomAlert1();" // 2 秒后关闭第一个弹窗
                    "setTimeout(function() {"
                    "fileUpload_showCustomAlert2();" // 再等待 5 秒后显示第二个弹窗
                    "}, 1500);" // 5 秒延迟
                    "}, 6000);" // 2 秒延迟
                    "}, 1000);" // 1 秒延迟
                    "var formData = new FormData();"
                    "formData.append('file', files[0]);"
                    "var xhr = new XMLHttpRequest();"
                    "xhr.open('POST', '/upload', true);"
                    "xhr.onload = function() {"
                    "if (xhr.status !== 200) {"
                    "alert('文件上传失败，请稍后再试！');"
                    "}"
                    "};"
                    "xhr.send(formData);"
                    "}"
                    "function fileUpload_showCustomAlert1() {"
                    "document.getElementById('fileUpload_customAlert1').style.display = 'flex';"
                    "}"
                    "function fileUpload_closeCustomAlert1() {"
                    "document.getElementById('fileUpload_customAlert1').style.display = 'none';"
                    "}"
                    "function fileUpload_showCustomAlert2() {"
                    "document.getElementById('fileUpload_customAlert2').style.display = 'flex';"
                    "}"
                    "function fileUpload_closeCustomAlert2() {"
                    "document.getElementById('fileUpload_customAlert2').style.display = 'none';"
                    "}"
                "</script>"

                // 按钮的悬停效果样式
                "<style>"
                    ".btn-upload1:hover {"
                    "background-color: #0056b3;"
                    "box-shadow: 0 8px 12px rgba(0, 123, 255, 0.4);"
                    "transform: scale(1.05);"
                    "}"
                    ".btn-download1:hover {"
                    "background-color: #218838;"
                    "box-shadow: 0 8px 12px rgba(40, 167, 69, 0.4);"
                    "transform: scale(1.05);"
                    "}"
                "</style>"
            "</div>"
        "</td></tr>");
}

// 生成关于部分
void generateAboutSection(char *buf) {
    // 拼接关于工具的 HTML 代码
    sprintf(buf,
        // 关于工具部分
        "<tr><td colspan=\"3\">"
            "<div style=\"background: linear-gradient(135deg, #f9f9f9, #e0e0e0); "
            "padding: 30px; border-radius: 15px; box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15); "
            "margin-top: 30px; margin-bottom: 20px; animation: fadeIn 0.5s ease-out; "
            "transition: transform 0.3s ease, box-shadow 0.3s ease;\" "
            "onmouseover=\"this.style.transform='translateY(-8px)'; this.style.boxShadow='0 10px 30px rgba(0, 0, 0, 0.2)';\" "
            "onmouseout=\"this.style.transform='translateY(0)'; this.style.boxShadow='0 8px 24px rgba(0, 0, 0, 0.15)';\">"
                "<h2 style=\"font-family: 'Arial', sans-serif; color: #333; margin-bottom: 20px; "
                "border-bottom: 2px solid #007bff; display: inline-block; padding-bottom: 5px;\">"
                "关于工具"
                "</h2>"
                "<p style=\"font-size: 16px; color: #555; line-height: 1.8; margin-bottom: 20px;\">"
                "本工具是一款高效的真菌物种注释平台，支持物种名称检索及多格式数据文件上传分析，相关工程文件和使用文档已开源发布于 GitHub。"
                "工具内置的深度学习模型基于大规模公开真菌 ITS 数据集训练，涵盖广泛物种，具备高分类准确性和泛化能力，适用于真菌分类学及生态学研究等领域。"
                "</p>"
                "<div style=\"text-align: center;\">"
                    "<a href=\"https://github.com/liukai2410/fungilt\" target=\"_blank\" "
                    "style=\"display: inline-block; padding: 14px 28px; background-color: #007bff; "
                    "color: white; font-size: 16px; border: none; border-radius: 12px; cursor: pointer; "
                    "text-decoration: none; box-shadow: 0 6px 8px rgba(0, 123, 255, 0.2); "
                    "transition: background-color 0.3s ease, box-shadow 0.3s ease, transform 0.3s ease;\" "
                    "onmouseover=\"this.style.backgroundColor='#0056b3'; this.style.boxShadow='0 8px 12px rgba(0, 123, 255, 0.3)'; "
                    "this.style.transform='scale(1.05)';\" "
                    "onmouseout=\"this.style.backgroundColor='#007bff'; this.style.boxShadow='0 6px 8px rgba(0, 123, 255, 0.2)'; "
                    "this.style.transform='scale(1)';\">"
                    "查看 GitHub 仓库"
                    "</a>"
                "</div>"
            "</div>"
        "</td></tr>"

        // 使用流程部分
        "<tr><td colspan=\"3\">"
            "<div style=\"background: linear-gradient(135deg, #f9f9f9, #e0e0e0); "
            "padding: 30px; border-radius: 15px; box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15); "
            "margin-top: 20px; margin-bottom: 30px; animation: fadeIn 0.5s ease-out; "
            "transition: transform 0.3s ease, box-shadow 0.3s ease;\" "
            "onmouseover=\"this.style.transform='translateY(-8px)'; this.style.boxShadow='0 10px 30px rgba(0, 0, 0, 0.2)';\" "
            "onmouseout=\"this.style.transform='translateY(0)'; this.style.boxShadow='0 8px 24px rgba(0, 0, 0, 0.15)';\">"
                "<h2 style=\"font-family: 'Arial', sans-serif; color: #333; margin-bottom: 20px; "
                "border-bottom: 2px solid #28a745; display: inline-block; padding-bottom: 5px;\">"
                "使用流程"
                "</h2>"
                "<div style=\"display: flex; justify-content: space-between; flex-wrap: wrap;\">"
                    // 步骤 1
                    "<div style=\"flex: 1; min-width: 250px; margin: 15px; padding: 25px; "
                    "background-color: #ffffff; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); "
                    "text-align: center; transition: transform 0.3s ease, box-shadow 0.3s ease;\" "
                    "onmouseover=\"this.style.transform='translateY(-8px)'; this.style.boxShadow='0 8px 16px rgba(0, 0, 0, 0.2)';\" "
                    "onmouseout=\"this.style.transform='translateY(0)'; this.style.boxShadow='0 4px 8px rgba(0, 0, 0, 0.1)';\">"
                        "<h3 style=\"font-size: 18px; color: #007bff; margin-bottom: 10px;\">"
                        "<i class=\"fa fa-upload\" style=\"font-size: 24px; margin-right: 10px;\"></i>步骤 1"
                        "</h3>"
                        "<p style=\"font-size: 14px; color: #555;\">输入物种名称或上传数据文件。</p>"
                    "</div>"
                    // 步骤 2
                    "<div style=\"flex: 1; min-width: 250px; margin: 15px; padding: 25px; "
                    "background-color: #ffffff; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); "
                    "text-align: center; transition: transform 0.3s ease, box-shadow 0.3s ease;\" "
                    "onmouseover=\"this.style.transform='translateY(-8px)'; this.style.boxShadow='0 8px 16px rgba(0, 0, 0, 0.2)';\" "
                    "onmouseout=\"this.style.transform='translateY(0)'; this.style.boxShadow='0 4px 8px rgba(0, 0, 0, 0.1)';\">"
                        "<h3 style=\"font-size: 18px; color: #007bff; margin-bottom: 10px;\">"
                        "<i class=\"fa fa-cogs\" style=\"font-size: 24px; margin-right: 10px;\"></i>步骤 2"
                        "</h3>"
                        "<p style=\"font-size: 14px; color: #555;\">等待分析完成。</p>"
                    "</div>"
                    // 步骤 3
                    "<div style=\"flex: 1; min-width: 250px; margin: 15px; padding: 25px; "
                    "background-color: #ffffff; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); "
                    "text-align: center; transition: transform 0.3s ease, box-shadow 0.3s ease;\" "
                    "onmouseover=\"this.style.transform='translateY(-8px)'; this.style.boxShadow='0 8px 16px rgba(0, 0, 0, 0.2)';\" "
                    "onmouseout=\"this.style.transform='translateY(0)'; this.style.boxShadow='0 4px 8px rgba(0, 0, 0, 0.1)';\">"
                        "<h3 style=\"font-size: 18px; color: #007bff; margin-bottom: 10px;\">"
                        "<i class=\"fa fa-download\" style=\"font-size: 24px; margin-right: 10px;\"></i>步骤 3"
                        "</h3>"
                        "<p style=\"font-size: 14px; color: #555;\">下载分析结果。</p>"
                    "</div>"
                "</div>"
            "</div>"
        "</td></tr>"
    );
}




// 生成网页页脚部分的HTML
void generateFooterSection(char *buf)
{
    // 拼接页脚HTML代码
    sprintf(buf, 
        // 页脚区域
        "<tr><td colspan=\"3\"><div class=\"footer-section\" style=\"background: linear-gradient(135deg, #1a3c6c, #003366); color: white; padding: 20px 10px; text-align: center; font-size: 14px; margin-left: -8px; border-top: 2px solid #fff; border-radius: 8px;\">"
    
        // 地址
        "<div style=\"margin-bottom: 14px; font-size: 16px; line-height: 1.4;\">"
        "<strong>地址：</strong>江苏省无锡市蠡湖大道1800号江南大学协同创新中心C501-517"
        "</div>"
        
        // 邮编
        "<div style=\"margin-bottom: 14px; font-size: 16px; line-height: 1.4;\">"
        "<strong>邮编：</strong>214122"
        "</div>"
        
        // 联系方式
        "<div style=\"margin-bottom: 14px; font-size: 16px; line-height: 1.4;\">"
        "<strong>联系方式：</strong>+86 0510 85329063"
        "</div>"
        
        // 邮箱链接
        "<div style=\"margin-bottom: 14px; font-size: 16px; line-height: 1.4;\">"
        "<strong>邮箱：</strong><a href=\"mailto:rctff@jiangnan.edu.cn\" style=\"color: #f9f9f9; text-decoration: none; transition: color 0.3s ease;\">rctff@jiangnan.edu.cn</a>"
        "</div>"
        
        // 版权声明
        "<div style=\"margin-top: 14px; font-size: 12px; color: #bbb; line-height: 1.4;\">"
        "© 2025 江南大学 | 所有权利保留"
        "</div>"
    
        "</div></td></tr>"
    );
    
    
}