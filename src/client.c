#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <termio.h>
#include <pthread.h>
#include <dirent.h>

#include "client.h"/* .c文件默认会寻找同名的.h文件，可以不加 */
#include "./inc/sha1.h"
 


int g_client_tcp_sockfd = -1;                       /* 全局tcp socket  负值为sockfd有问题 */
int g_client_udp_sockfd = -1;                       /* 全局udp socket  负值为sockfd有问题 */
struct sockaddr_in g_udp_sockaddr;                  /* 获取全局的UDP新服务端处理端口， 非监听端口19977 */
char g_place_of_file[BUFLEN];                       /* 全局定义客户端文件的下载位置 可用于切换路径 */






int main(int argc, char*argv)
{

    /* 默认在同目录下创建一个client_file文件夹用于下载 */
    //文件夹末尾一定要加'/'
    //文件夹末尾一定要加'/'
    //文件夹末尾一定要加'/'
    strcpy(g_place_of_file, "./client_file/");
    mk_place_of_file(g_place_of_file);


    init_sys_command();

    char input[BUFLEN];
    while(true)
    {
        cmd_pre();
        fgets(input,BUFLEN,stdin);
        if(input[strlen(input)-1] == '\n')
        {
            input[strlen(input)-1] = '\0';
        }
        //解析输入的命令
        switch(analysis(input))
        {
            case Sys_Help:  
                helpmenu();
                break;   
            case Sys_Connect:
                client_connect(cmd);
                break;
            case Sys_Download:
                client_download_pack(cmd);
                break;
            case Sys_Upload:
                client_upload_pack(cmd);
                break;
            case Sys_Scan:
                udp_scan();
                break;
            case Sys_Clear:
                system("clear");
                break;
            case Sys_Quit:
                close(g_client_tcp_sockfd);
                return 0;
            case Sys_Ls:
                ls_func();
                break;
            case -1:
                printf("Please Input At Least One Command!\r\n");
                break;
            case -2:
                printf("Can't Analysis This Command!\r\n");
                break;
            default:
                break;
        }

    }
}


/* 显示cmd前置 */
void cmd_pre()
{
    printf("\033[1;32mBot@localhost:~\033[0m");
    printf("\033[0;34m$ \033[0m");
}


/* 初始化SysCommand */
void init_sys_command()
{
    strcpy(SysCommand[0], "help");
    strcpy(SysCommand[1], "connect");
    strcpy(SysCommand[2], "download");
    strcpy(SysCommand[3], "upload");
    strcpy(SysCommand[4], "scan");
    strcpy(SysCommand[5], "clear");
    strcpy(SysCommand[6], "quit");
    strcpy(SysCommand[7], "ls");
}


/* 帮助菜单信息打印 */
void helpmenu()
{
    printf("command: \r\n\
    help        ---  show help menu \r\n\
    connect     ---  connect to a server; Usage: 'connect IpAddress \r\n\
    download    ---  download from server; Usage: 'download TCP/UDP filename'\r\n\
    upload      ---  upload to server; Usage: 'upload TCP/UDP filename'\r\n\
    scan        ---  scan online server; only can scan same segment\r\n\
    clear       ---  clear the screen \r\n\
    ls          ---  show server dir;file you can download\r\n\
    quit        ---  exit this system\r\n\n\
    \033[1;32mall commands are Case-Insensitive\r\n\n\033[0m"
    );
}


/* 解析输入的命令
*若输入超过3个参数，后面的参数将被丢弃
*/
int analysis(char* str)
{
    //命令行最多输入3个参数，每个参数最多19个字符
    for(int i = 0;i < 3;i++)
    {
        cmd[i][0] = '\0';
    }
    sscanf(str, "%s %s %s", cmd[0], cmd[1], cmd[2]);
    for(int i = 0; i < MAX_COMAND_NUM; i++)
    {
        if(strcasecmp(cmd[0], SysCommand[i]) == 0)
        {
            return i;
        }
    }
    if(strcasecmp(cmd[0], "") == 0)
    {
        return -1;//空输入；
    }
     return -2;//错误输入；
}





//工具函数
/* 获取文件的大小信息 
* 返回值为文件大小
*/
int get_file_size(char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    int size=statbuf.st_size;
 
    return size;
}


/* 无回显读取字符 */
int getch(void)
{
    struct termios tm, tm_old;
    int fd = 0, ch;
    /* 保存现在的终端设置 */
    if(tcgetattr(fd, &tm) < 0) 
    {
        return -1;
    }
    tm_old = tm;
    /* 更改终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理 */
    cfmakeraw(&tm);
    /* 设置上更改之后的设置 */
    if(tcsetattr(fd, TCSANOW, &tm) < 0) 
    {
        return -1;
    }
 
    ch = getchar();

    /* 更改设置为最初的样子 */
    if(tcsetattr(fd, TCSANOW, &tm_old) < 0) 
    {
        return -1;
    }
    
    return ch;
}


/* 从路径中分离出单个文件名 */
char *get_file_single_name(char *filefullname)
{
    int x = strlen(filefullname);
    char ch = '/';
    char *filesiglename = strrchr(filefullname,ch) + 1;

    return filesiglename;
}


/* 创建文件存放路径 */
void mk_place_of_file(char * file_dir)
{
    if(NULL == opendir(file_dir))
    {
        mkdir(file_dir,0775);
    }
}





/*传输相关函数*/
/* udp组播，用于客户端探测服务端 
*采用局部套接字，scan一次创建一个，结束关闭套接字
*C:scan request
*S:scan response 200 OK
*/
int udp_scan()
{
    printf("\033[1;32m~~ Press 'q' to jump out the scan module ~~\033[0m\r\n");
    //局部套接字，结束即关闭
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);         /*建立套接字*/
    if(-1 == sockfd)
    {
        printf("socket error (func socket)\r\n");
        return -1;
    }


    /* 初始化地址 */
    struct sockaddr_in mcast_addr; 
    memset(&mcast_addr, 0, sizeof(mcast_addr));/*初始化IP多播地址为0*/
    mcast_addr.sin_family = AF_INET;                /*设置协议族类行为AF*/
    mcast_addr.sin_addr.s_addr = inet_addr(MCAST_ADDR);/*设置多播IP地址*/
    mcast_addr.sin_port = htons(MCAST_PORT);        /*设置多播端口*/

    /*向多播地址发送数据*/
    int send_len = sendto(sockfd, MCAST_DATA, sizeof(MCAST_DATA), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr)) ;
    if(send_len < 0)
    {
        printf ("sendto error (func sendto)\r\n");
        return -2;
    }

    /*发送后开始循环接受服务端19971端口发来的消息，不管什么消息都记录
    *有消息即代表服务端在线
    */
    pthread_t mcast_msg_thread;
    pthread_create(&mcast_msg_thread, NULL, (void *)print_udp_scan_result, &sockfd);
    pthread_detach(mcast_msg_thread);//关闭线程即释放资源

    /* recvfrom 和 getch 都为阻塞函数，因此决定另起线程用于接收显示
    *主线程用于接收用户输入
    */
    while(true)
    {
        if(getch() == 'q')
        {
            /* 经测试，不会造成线程不退出的情况 */
            pthread_cancel(mcast_msg_thread);//发出信号取消线程
            return 0;
        }
    }
}


/* 输出udp探测结果的消息 位于子线程中 */
void print_udp_scan_result(void * param)
{
    int sockfd = *((int *)param);
    /* 记录在线服务端信息 */
    struct sockaddr_in online_server_addr;
    /* 接收的消息buf */
    char recv_msg[BUFLEN + 1];
    /* 接收buf的实际大小 */
    unsigned int recv_size = 0;
    unsigned int addr_len = sizeof(struct sockaddr_in);

    //设置立即取消 recvfrom
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    while(true)
    {      
        memset(recv_msg, 0x00, BUFLEN + 1);
        recv_size = recvfrom(sockfd, recv_msg, BUFLEN, 0, (struct sockaddr *) &online_server_addr, &addr_len);
        if(recv_size < 0 || 0 == recv_size)
        {
            printf("recvfrom error (func recvfrom)\r\n");
            pthread_exit(0);
            return;
        }
        else
        {
        /* 成功接收到数据报,末尾加结束符 */
            recv_msg[recv_size] = '\0';

            /* 获取ip和port */
            char ip_str[INET_ADDRSTRLEN];   /* INET_ADDRSTRLEN这个宏系统默认定义 16 */
            inet_ntop(AF_INET,&(online_server_addr.sin_addr), ip_str, sizeof(ip_str));
            int port = ntohs(online_server_addr.sin_port);

            printf("%s--%d : [%s]\r\n", ip_str, port, recv_msg);
        }
    }
}


/*初始化一个tcp socket
*参数为用户输入的可用服务端的ip地址
*返回值为创建的该socket，用于赋值给g_client_tcp_sockfd 
*/
int client_tcp_sockfd_init(char* ip_addr)
{
    /* 创建socket */
    int sockfd;
    if(-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        return -1;
    }

    /* 初始化地址 */
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    server_addr.sin_port = htons(TCP_TRANS_PORT);

    if(connect(sockfd,(struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        return -2;
    }

    return sockfd;
}


/*初始化一个udp socket
*参数为用户输入的可用服务端的ip地址
*返回值为创建的该socket，用于赋值给g_client_udp_sockfd 
*/
int client_udp_sockfd_init(char* ip_addr)
{
    struct sockaddr_in server_addr;
    int sockfd;
    char first_msg[BUFLEN] = "udp client come"; 
    int addr_len = sizeof(struct sockaddr_in);

    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(-1 == sockfd)
    {
        return -1;
    }
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    server_addr.sin_port = htons(UDP_TRANS_PORT);  

    sendto(sockfd, first_msg, strlen(first_msg), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    memset(first_msg, 0, sizeof(first_msg));//清空first_msg
    //获取新的服务端的端口地址信息
    int n = recvfrom(sockfd, first_msg, BUFLEN, 0, (struct sockaddr *)&server_addr, &addr_len);//此时first_msg里的值为New Server Here
    if(-1 == n)
    {
        //接收失败
        return -1;
    }
    /*成功获取了新的服务端的端口地址信息后，可以进行进一步操作如upload和download
    *将服务端的server_addr保存到g_udp_sockaddr
    */
    g_udp_sockaddr = server_addr;//浅拷贝，但是够用了
    return sockfd;
}


/* 客户端连接服务端 包括tcp和udp 
* 传入的参数为用户命令行输入的指令
*/
void client_connect(char mcmd[][20])
{
    /* 如果缺少第二个参数 */
    if(0 == strcasecmp(mcmd[1],""))
    {
        printf("\033[1;31mUsage: 'connect IpAddress'\r\n\033[0m");
    }
    else/* 输入基本格式正确 还有对IP的判断没有实现*/
    {
        //tcp
        close(g_client_tcp_sockfd);//先关闭socket
        if(0 > (g_client_tcp_sockfd = client_tcp_sockfd_init(mcmd[1]))) //sockfd存在问题
        {
            printf("\033[1;31mTcp Can't connect! Check if server online!\033[0m\r\n");
        }
        else
        {
            printf("\033[1;32mA new Tcp connection established! old one closed!\r\n\033[0m");
        }

        //udp
        close(g_client_udp_sockfd);
        memset(&g_udp_sockaddr,0,sizeof(g_udp_sockaddr));
        if(0 > (g_client_udp_sockfd = client_udp_sockfd_init(mcmd[1]))) //sockfd存在问题
        {
            printf("\033[1;31mGet Udp Server Fail! Check if server online!\033[0m\r\n");
        }
        else
        {
            printf("\033[1;32mGet Udp Server Succeed! UDP action Can Use!\r\n\033[0m");
        }

    }
}


/* 客户端上传服务端 包括tcp和udp 
* 传入的参数为用户命令行输入的指令
*/
void client_upload_pack(char mcmd[][20])
{
    /* tcp 模式 */
    if(0 == strcasecmp(mcmd[1], "tcp")) 
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename 为空
        {
            printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
        }
        else//输入基本格式正确
        {
            if(0 > g_client_tcp_sockfd)//检查sockfd是否正常
            {
                printf("\033[1;31mPlease connect a server first!!!\r\n\033[0m");
            }
            else //tcp
            {
                if((access(mcmd[2],F_OK)) != -1)//判断资源是否存在
                {
                    struct stat s_buf;
                    stat(mcmd[2], &s_buf);//获取资源信息
                    if(S_ISREG(s_buf.st_mode))//判断是file还是dir
                    {
                       client_tcp_upload(mcmd[2]); 
                    }
                    else
                    {
                        printf("\033[1;31mCan't Uplaod a Dir!!!\r\n\033[0m");
                    }
                }
                else
                {
                    printf("\033[1;31mPlease check if file is exist!!!\r\n\033[0m");
                }
            }
        }
    }
    /* udp 模式 */
    else if(0 == strcasecmp(mcmd[1],"udp"))
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename 为空
        {
            printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
        }
        else//输入基本格式正确
        {
            if(0 > g_client_udp_sockfd)//检查sockfd是否正常
            {
                printf("\033[1;31mPlease Check If UDP Server online!!!\r\n\033[0m");
            }
            else //tcp
            {
                if((access(mcmd[2],F_OK)) != -1)//判断资源是否存在
                {
                    struct stat s_buf;
                    stat(mcmd[2], &s_buf);//获取资源信息
                    if(S_ISREG(s_buf.st_mode))//判断是file还是dir
                    {
                       client_udp_upload(mcmd[2]); 
                    }
                    else
                    {
                        printf("\033[1;31mCan't Uplaod a Dir!!!\r\n\033[0m");
                    }
                }
                else
                {
                    printf("\033[1;31mPlease check if file is exist!!!\r\n\033[0m");
                }
            }
        }
    }
    /* 输入错误 */
    else
    {
        printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
    }
}


/* tcp模式下上传文件到服务器 
* socket 采用g_client_tcp_sockfd
*因此在使用前需要注意 该socket是否已经正确初始化
*消息格式为
*  " Message_Type, FileName, FileSize, HashCode "
*upload 的文件名 可以是根据本地路径来选择 如./dir/test.txt
*/
int client_tcp_upload(char* filename)
{
    /*对文件名处理，同一先改成路径类型的，如输入data需要转换为./data
    *系统决定文件名不允许出现 '/'
    */
    char new_filename[BUFLEN] = {0};//用于存储新的路径类型文件名
    if(NULL == strchr(filename, '/'))//没找到
    {
        sprintf(new_filename,"./%s", filename);
    }
    else
    {
        strcpy(new_filename, filename);
    }

    /* 文件信息结构体初始化 */
    struct FileInfo m_fileinfo;
    memset(&m_fileinfo, 0, sizeof(m_fileinfo));
    strcpy(m_fileinfo.FileName,new_filename);
    m_fileinfo.FileSize = get_file_size(m_fileinfo.FileName);
    strcpy(m_fileinfo.HashCode, get_sha1_from_file(m_fileinfo.FileName));


    char str[BUFLEN] = {0};
    sprintf(str,"%d,%s,%d,%s", Message_Tcp_Upload, get_file_single_name(m_fileinfo.FileName), m_fileinfo.FileSize, m_fileinfo.HashCode);
    printf("Sha1 Code: %s\r\n", m_fileinfo.HashCode);

    //发送要上传的文件信息
    send(g_client_tcp_sockfd, str, strlen(str), 0);

    //接收服务端发来的是否存在断点文件消息
    recv(g_client_tcp_sockfd, str, BUFLEN, 0);//传送格式为 从什么位置开始传的int值,无文件则为0，有断点文件则为大于0的值
    int from_where = atoi(str);

    //选择是从断点处发还是新发送
    FILE *fp;
    fp = fopen(m_fileinfo.FileName,"rb");
    if(fp == NULL)
    {
        return -3;
    }

    char buf[BUFLEN + 1] = {0};
    fseek(fp, from_where, SEEK_SET);//从服务端传来的开始处开始
    int nLen = 0;//读取长度
    int nSize = 0;//发送长度
    int sdCount = 0;//记录发送了多少
    printf("Uploading....\r\n");
    while(true)
    {
        nLen = fread(buf, 1, BUFLEN, fp);
        if(nLen == 0)
        {
            break;
        }
        nSize = send(g_client_tcp_sockfd, (const char *)buf, nLen, 0);
        sdCount += nSize;
        printf("%d of %d has sended!\r\n", sdCount, m_fileinfo.FileSize - from_where);
    }
    printf("\033[1;32mSend complete! buf size = %dbytes\r\n\033[0m", sdCount);
    fclose(fp);

    /* 等待服务端回传消息 验证文件完整 */
    if(0 == recv(g_client_tcp_sockfd, (char *)buf, BUFLEN, 0))
    {
        //服务端传回出错
        return 0;
    }
    printf("Server: %s\r\n", buf);

    return 0;
}


/* udp模式下上传文件到服务器 
* socket 采用g_client_udp_sockfd
*因此在使用前需要注意 该socket是否已经正确初始化
*消息格式为
*  " Message_Type, FileName, FileSize, HashCode "
*upload 的文件名 可以是根据本地路径来选择 如./dir/test.txt
*/
int client_udp_upload(char* filename)
{
    /*对文件名处理，同一先改成路径类型的，如输入data需要转换为./data
    *系统决定文件名不允许出现 '/'
    */
    char new_filename[BUFLEN] = {0};//用于存储新的路径类型文件名
    if(NULL == strchr(filename, '/'))//没找到
    {
        sprintf(new_filename,"./%s", filename);
    }
    else
    {
        strcpy(new_filename, filename);
    }

    /* 文件信息结构体初始化 */
    struct FileInfo m_fileinfo;
    memset(&m_fileinfo, 0, sizeof(m_fileinfo));
    strcpy(m_fileinfo.FileName,new_filename);
    m_fileinfo.FileSize = get_file_size(m_fileinfo.FileName);
    strcpy(m_fileinfo.HashCode, get_sha1_from_file(m_fileinfo.FileName));


    char str[BUFLEN] = {0};
    sprintf(str,"%d,%s,%d,%s", Message_Udp_Upload, get_file_single_name(m_fileinfo.FileName), m_fileinfo.FileSize, m_fileinfo.HashCode);
    printf("Sha1 Code: %s\r\n", m_fileinfo.HashCode);

    //发送要上传的文件信息
    sendto(g_client_udp_sockfd, str, strlen(str), 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));

    //打开文件准备开始发送文件
    FILE *fp;
    fp = fopen(m_fileinfo.FileName,"rb");
    if(fp == NULL)
    {
        return -3;
    }

    char buf[BUFLEN + 1] = {0};
    int nLen = 0;//读取长度
    int nSize = 0;//发送长度
    int sdCount = 0;//记录发送了多少
    int addr_len = sizeof(g_udp_sockaddr);
    printf("Uploading....\r\n");
    while(true)
    {
        nLen = fread(buf, 1, BUFLEN, fp);
        if(nLen == 0)
        {
            break;
        }
        nSize = sendto(g_client_udp_sockfd, (const char *)buf, nLen, 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));
        
        //等待服务端应答
        recvfrom(g_client_udp_sockfd, buf, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len);
        
        sdCount += nSize;
        printf("%d of %d has sended!\r\n", sdCount, m_fileinfo.FileSize);
    }
    //发送传输完成消息
    sendto(g_client_udp_sockfd, "OK", 2, 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));

    printf("\033[1;32mSend complete! buf size = %dbytes\r\n\033[0m", sdCount);
    fclose(fp);

    /* 等待服务端回传消息 验证文件完整 */
    memset(str, 0, sizeof(buf));
    if(0 == recvfrom(g_client_udp_sockfd, buf, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len))
    {
        //服务端传回出错
        return 0;
    }
    printf("Server: %s\r\n", buf);

    return 0;
}


/* 客户端从服务端下载 包括tcp和udp 
* 传入的参数为用户命令行输入的指令
*/
void client_download_pack(char mcmd[][20])
{
    /* tcp 模式 */
    if(0 == strcasecmp(mcmd[1], "tcp")) 
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename 为空
        {
            printf("\033[1;31mUsage: 'download TCP/UDP filename'\r\n\033[0m");
        }
        else//输入基本格式正确
        {
            if(0 > g_client_tcp_sockfd)//检查sockfd是否正常
            {
                printf("\033[1;31mPlease connect a server first!!!\r\n\033[0m");
            }
            else //tcp
            {
                client_tcp_download(mcmd[2]);
            }
        }
    }
    /* udp 模式 */
    else if(0 == strcasecmp(mcmd[1],"udp"))
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename 为空
        {
            printf("\033[1;31mUsage: 'download TCP/UDP filename'\r\n\033[0m");
        }
        else//输入基本格式正确
        {
            if(0 > g_client_tcp_sockfd)//检查sockfd是否正常
            {
                printf("\033[1;31mPlease Check If UDP Server online!!!\r\n\033[0m");
            }
            else //tcp
            {
                client_udp_download(mcmd[2]);
            }
        }
    }
    /* 输入错误 */
    else
    {
        printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
    }
}


/* tcp模式下从服务器下载文件
* socket 采用g_client_tcp_sockfd
*因此在使用前需要注意 该socket是否已经正确初始化
*消息格式为
*  " Message_Type, FileName, FileSet, HashCode "
* FileSet 一般为0 当有断点文件时为size值
* HashCode 一般为0 当有断点文件时为hashcode值
* 断点文件结构是 size \r\n hashcode
* filename 只允许是单一的文件名，不可以包含路径，即只允许从server端指定的文件夹下下载
*/
int client_tcp_download(char *filename)
{
    //客户端的判断； 在服务端也有做判断 
    if(NULL != strchr(filename, '/'))//如果是路径类型的文件名
    {
        printf("\033[1;31mYour request is illegal!!!\r\n\033[0m");
        return 0;
    }
    if(!strcmp(filename, ".") || !strcmp(filename, ".."))
    {
        printf("\033[1;31mYour request is illegal!!!\r\n\033[0m");
        return 0;
    }


    char tmp_size[BUFLEN] = {0};
    strcpy(tmp_size, "0");
    char tmp_hashcode[BUFLEN] = {0};
    strcpy(tmp_hashcode, "0");

    char save_file[BUFLEN] = {0};
    sprintf(save_file, "%s%s", g_place_of_file, filename);

    //先找是否存在断点文件
    char b_file[BUFLEN];
    sprintf(b_file, "%s.%s.bot.break", g_place_of_file, filename);
    if((access(b_file,F_OK)) != -1)//文件存在
    {
        FILE *fp_b;
        fp_b = fopen(b_file,"r");
        fgets(tmp_size, BUFLEN, fp_b);
        tmp_size[strlen(tmp_size) -1] = '\0';//去掉\r\n
        fgets(tmp_hashcode, BUFLEN, fp_b);
        fclose(fp_b);
    }

    char str[BUFLEN] = {0};
    sprintf(str, "%d,%s,%d,%s", Message_Tcp_Download, filename, atoi(tmp_size), tmp_hashcode);
    //发送下载请求给服务端，格式为msg_type, filename, fileset,hashcode
    send(g_client_tcp_sockfd, str, strlen(str), 0);
    //接收服务端回传的文件基本信息，格式为 is_exist,hashcode,filesize  对第一个参数 0:文件不存在 1：不续传 2：续传
    recv(g_client_tcp_sockfd, str, BUFLEN, 0);
    
    /* 分割str信息 存在m_msg里 */
    char seg[] = ",";
    char m_msg[3][BUFLEN];
    int i = 0;
    char *substr = strtok(str,seg);
    while(substr != NULL)
    {
        strcpy(m_msg[i],substr);
        ++i;
        substr = strtok(NULL,seg);
    }

    if(0 == atoi(m_msg[0]))//请求的文件不存在
    {
        printf("Your Requested File \033[1;31mNot Exist!!!\r\n\033[0m");
        //如果请求的文件不存在则不管有无断点文件都不删除该断点文件
        return 0;
    }
    else//请求的文件存在，验证hash值，相同则续传，不同则覆盖，两者都要删除断点文件，
    {
        //如果请求的文件存在且该文件有断点文件，则删除该断点文件
        if(0 != strcmp(tmp_hashcode, "0"))//验证是否存在断点文件 
        {
            remove(b_file);//有断点则删除
        }

        FILE *fp;
        if(1 == atoi(m_msg[0]))//无需续传
        {
            fp = fopen(save_file, "wb+");//二进制方式打开 擦除源文件
            strcpy(tmp_size, "0");//针对文件覆盖需要重置为0
        }
        else//2 则需要续传
        {
            fp = fopen(save_file, "rb+");//二进制方式打开 不擦除源文件
        }


        int dwCount = 0;//用于统计总字节数
        fseek(fp, atoi(tmp_size), SEEK_SET);//跳转到指定位置
        while(true)
        {
            char buf[BUFLEN] = {0};
            int length = recv(g_client_tcp_sockfd, (char *)buf, BUFLEN, 0);
            if(0 == length)//服务端断开，产生断点文件
            {
                fclose(fp);

                //创建断点文件 格式为.filename.bot.break 隐藏，对外界面不可见
                FILE *fp_break;
                //对同名文件的断点续传 只处理后面的续传，前面的被后面的同名文件覆盖了
                fp_break = fopen(b_file, "w+");
                char str_dwCount[BUFLEN];
                sprintf(str_dwCount, "%d", dwCount);
                fwrite(str_dwCount, 1, strlen(str_dwCount), fp_break);//dwcount
                fwrite("\r\n", 1, 2, fp_break);//换行
                fwrite(m_msg[1], 1, strlen(m_msg[1]), fp_break);//hashcode
                fclose(fp_break);

                break;
            }

            fwrite(buf, 1, length, fp);

            dwCount += length;
            printf("%d of %d has reveived!\r\n", dwCount, atoi(m_msg[2]) - atoi(tmp_size));
            if(dwCount == (atoi(m_msg[2]) - atoi(tmp_size)))
            {
                /* 写入字节数相同,但可能存在包被篡改情况,需校验hashcode */
                fclose(fp);
                char HashCode[41];
                strcpy(HashCode, get_sha1_from_file(save_file));//自己计算写入完成的文件的hashcode
                if(0 == strcmp(HashCode, m_msg[1]))
                {
                    if(1 == atoi(m_msg[0]))//非续传返回
                    {
                        printf("File Complete True; Sha1: %s\r\n", HashCode);
                    }
                    else//续传返回
                    {
                        printf("Continue File Complete True; Sha1: %s\r\n", HashCode);
                    }
                }
                else
                {
                    if(1 == atoi(m_msg[0]))//非续传返回
                    {
                        printf("File Complete False; Sha1: %s\r\n", HashCode);
                    }
                    else//续传返回
                    {
                        printf("Continue File Complete False; Sha1: %s\r\n", HashCode);
                    }
                }
                break;
            }
        }
    }

    return 0;
}


/* udp模式下从服务器下载文件
* socket 采用g_client_udp_sockfd
*因此在使用前需要注意 该socket是否已经正确初始化
*消息格式为
*  " Message_Type, FileName "
* filename 只允许是单一的文件名，不可以包含路径，即只允许从server端指定的文件夹下下载
*/
int client_udp_download(char *filename)
{
    //客户端的判断； 在服务端也有做判断 
    if(NULL != strchr(filename, '/'))//如果是路径类型的文件名
    {
        printf("\033[1;31mYour request is illegal!!!\r\n\033[0m");
        return 0;
    }
    if(!strcmp(filename, ".") || !strcmp(filename, ".."))
    {
        printf("\033[1;31mYour request is illegal!!!\r\n\033[0m");
        return 0;
    }



    char save_file[BUFLEN] = {0};
    sprintf(save_file, "%s%s", g_place_of_file, filename);

    
    char tmp_response[BUFLEN] = "response";
    char str[BUFLEN] = {0};
    int addr_len = sizeof(g_udp_sockaddr);
    sprintf(str, "%d,%s", Message_Udp_Download, filename);

    //发送下载请求给服务端，格式为msg_type, filename, fileset,hashcode
    sendto(g_client_udp_sockfd, str, strlen(str), 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));
    //接收服务端回传的文件基本信息，格式为 is_exist,hashcode,filesize  对第一个参数 0:文件不存在 1：存在
    recvfrom(g_client_udp_sockfd, str, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len);
    
    /* 分割str信息 存在m_msg里 */
    char seg[] = ",";
    char m_msg[3][BUFLEN];
    int i = 0;
    char *substr = strtok(str,seg);
    while(substr != NULL)
    {
        strcpy(m_msg[i],substr);
        ++i;
        substr = strtok(NULL,seg);
    }

    if(0 == atoi(m_msg[0]))//请求的文件不存在
    {
        printf("Your Requested File \033[1;31mNot Exist!!!\r\n\033[0m");
        //如果请求的文件不存在则不管有无断点文件都不删除该断点文件
        return 0;
    }
    else//请求的文件存在
    {
        FILE *fp;
        fp = fopen(save_file, "wb+");//二进制方式打开 擦除源文件

        int dwCount = 0;//用于统计总字节数
        while(true)
        {
            char buf[BUFLEN] = {0};
            int length = recvfrom(g_client_udp_sockfd, (char *)buf, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len);
            
            //服务端 发送文件完成
            if(0 == strcmp(buf, "OK"))
            {
                if(dwCount == atoi(m_msg[2]))
                {
                    /* 写入字节数相同,但可能存在包被篡改情况,需校验hashcode */
                    fclose(fp);
                    char HashCode[41];
                    strcpy(HashCode, get_sha1_from_file(save_file));//自己计算写入完成的文件的hashcode
                    if(0 == strcmp(HashCode, m_msg[1]))
                    {
                        printf("File Complete True; Sha1: %s\r\n", HashCode);
                    }
                    else
                    { 
                        printf("File Complete False; Sha1: %s\r\n", HashCode);
                    }
                    break;
                }
            }
            
            fwrite(buf, 1, length, fp);
            //写入完成 发送响应包
            sendto(g_client_udp_sockfd, tmp_response, BUFLEN, 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));

            dwCount += length;
            printf("%d of %d has reveived!\r\n", dwCount, atoi(m_msg[2]));
        }
    }

    return 0;
}



/* 显示服务端可下载的文件 */
void ls_func()
{
    if(0 > g_client_tcp_sockfd)//检查tcp sockfd是否正常
    {
        printf("\033[1;31mPlease connect a server first!!!\r\n\033[0m");
    }
    else //tcp
    {
        char str[2 * BUFLEN] = {0};
        sprintf(str, "%d,0", Message_Tcp_Ls);
        //发送ls命令给服务端，格式为msg_type, 0
        send(g_client_tcp_sockfd, str, strlen(str), 0);
        //接收服务端回传的文件基本信息，格式为 is_exist,hashcode,filesize  对第一个参数 0:文件不存在 1：不续传 2：续传
        recv(g_client_tcp_sockfd, str, 2 * BUFLEN, 0);
        printf("%s\r\n",str);
    }
}