/* printf用于单线程测试 */

#include <arpa/inet.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <dirent.h>

#include "server.h"/* .c文件默认会寻找同名的.h文件，可以不加 */
#include "./inc/sha1.h"

 

char g_place_of_file[BUFLEN];           /* 全局定义服务端文件的位置 可用于切换路径 */


int main(int argc, char **argv)
{

    /* 默认在同目录下创建一个server_file文件夹 */
    //文件夹末尾一定要加'/'
    //文件夹末尾一定要加'/'
    //文件夹末尾一定要加'/'
    strcpy(g_place_of_file, "./server_file/");
    mk_place_of_file(g_place_of_file);


    /* 
    *一个服务器进程启动时先创建3条线程监听端口 
    *分别为 
    *mcast_thread 监听udp广播
    *tcp_trans_thread 监听tcp传输  ---->  建立连接后 tcp_receive_thread 该线程用于处理数据
    *udp_trans_thread 监听udp传输
    */

    /* UDP广播端口监听线程 */
    pthread_t mcast_thread;
    pthread_create(&mcast_thread, NULL, (void *)udp_mcast_init, NULL);

    /* TCP传输端口监听线程 */
    int tcp_trans_sockfd = tcp_trans_init();
    if(tcp_trans_sockfd > 0)
    {
        pthread_t tcp_trans_thread;
        pthread_create(&tcp_trans_thread, NULL, (void *)recv_tcp_connect, &tcp_trans_sockfd);
    }

    /* UDP传输端口监听线程 */
    int udp_trans_sockfd = udp_trans_init();
    if(udp_trans_sockfd > 0)
    {
      pthread_t udp_trans_thread;
      pthread_create(&udp_trans_thread, NULL, (void *)recv_udp_connect, &udp_trans_sockfd);
    }


    /* 主线程 */
    char buf[1024];
    while(1)
    {
        scanf("%s",buf);
    }
}







/* 创建文件存放路径 */
void mk_place_of_file(char * file_dir)
{
    if(NULL == opendir(file_dir))
    {
        mkdir(file_dir,0775);
    }
}


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






/*
*服务端加入多播组
*多播组地址由宏定义UDP_MCAST_ADDR给出
*多播组端口由UDP_MCAST_PORT给出
*函数出错会返回 int类型 错误码
*否则将永远循环等待
*
*C:scan request
*S:scan response 200 OK
*/
int udp_mcast_init()
{
    /* 创建socket 用于UDP组播 针对服务器探测 */
    int sockfd; 
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        printf("socket creating error (func socket)\r\n");
        return(-1);
    }


    /* 初始化地址 */
    struct sockaddr_in local_mcast_addr;
    memset(&local_mcast_addr, 0, sizeof(local_mcast_addr));
    local_mcast_addr.sin_family = AF_INET;
    local_mcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_mcast_addr.sin_port = htons(UDP_MCAST_PORT);
    /* 绑定自己的端口和IP信息到socket上 */
    if(-1 == bind(sockfd, (struct sockaddr *) &local_mcast_addr, sizeof(local_mcast_addr)))
    {
        printf("bind error (func bind)\r\n");
        return(-2);
    }


    struct ip_mreq mreq;
    memset(&mreq, 0x00, sizeof(struct ip_mreq));
    /* 设置组播组地址 */
    mreq.imr_multiaddr.s_addr = inet_addr(UDP_MCAST_ADDR);
    /* 设置网络接口地址信息 */
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    /* 加入组播地址*/
    if(-1 == setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)))
    {
        printf("add membership error (func setsockopt)\r\n");
        return(-3);
    }


    /* 禁止数据回送到本地回环接口 */
    unsigned char loop = 0;
    if(-1 == setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,sizeof(loop)))
    {
        printf("set data loop error (func setsockopt)\r\n");
        return(-4);
    }
  

    /* 记录客户端信息 */
    struct sockaddr_in client_addr;
    /* 接收的消息buf */
    char recv_msg[BUFLEN + 1];
    /* 接收buf的实际大小 */
    unsigned int recv_size = 0;
    unsigned int addr_len = sizeof(struct sockaddr_in);
    /* 循环接收网络上来的组播消息 */
    while(true)
    {
        memset(recv_msg, 0x00, BUFLEN + 1);
        recv_size = recvfrom(sockfd, recv_msg, BUFLEN, 0, (struct sockaddr *) &client_addr, &addr_len);
        if(recv_size < 0)
        {
            printf("recvfrom error (func recvfrom)\r\n");
            return(-5);
        }
        else
        {
            /* 成功接收到数据报,末尾加结束符 */
            recv_msg[recv_size] = '\0';

            /* 获取ip和port */
            char ip_str[INET_ADDRSTRLEN];   /* INET_ADDRSTRLEN这个宏系统默认定义 16 */
            inet_ntop(AF_INET,&(client_addr.sin_addr), ip_str, sizeof(ip_str));
            int port = ntohs(client_addr.sin_port);

            printf ("%s--%d : [%s]\r\n", ip_str, port, recv_msg);
            //向对方回复数据，表示收到探测，不向组内其他成员发送数据
            char response[BUFLEN + 1] = "scan response 200 OK";
            if(sendto(sockfd, response, strlen(response), 0, (struct sockaddr *) &client_addr, addr_len) < 0)
            {
                printf ("sendto error (func sendto)\r\n");
                return(-6);
            }
        }
    }
    //return 0;
}


/*
*服务端TCP传输端口初始化
*返回正值为sockfd
*返回负值则出错
*listen队列长度由局部变量backlog指定
*后续sockfd相关由外部完成
*/
int tcp_trans_init()
{
    /* 创建socket 用于TCP传输 */
    int sockfd; 
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        printf("socket creating error (func socket)\r\n");
        return(-1);
    }


    /* 初始化地址 */
    struct sockaddr_in local_server_addr;
    memset(&local_server_addr, 0, sizeof(local_server_addr));
    local_server_addr.sin_family = AF_INET;
    local_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_server_addr.sin_port = htons(TCP_TRANS_PORT);
    /* 绑定自己的端口和IP信息到socket上 */
    if(-1 == bind(sockfd, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)))
    {
        printf("bind error (func bind)\r\n");
        return(-2);
    }


    //维护队列总和大小10000(已建立和未建立的套接字)
    int backlog = 10000;
    if(-1 == listen(sockfd,backlog))
    {
        printf("listen error (func listen)\r\n");
        return (-3);
    }
  

    return sockfd;
}


/*
*服务端UDP传输端口初始化
*返回正值为sockfd
*返回负值则出错
*后续sockfd相关由外部完成
*/
int udp_trans_init()
{
    /* 创建socket 用于UDP传输 */
    int sockfd; 
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        printf("socket creating error (func socket)\r\n");
        return(-1);
    }


    /* 初始化地址 */
    struct sockaddr_in local_server_addr;
    memset(&local_server_addr, 0, sizeof(local_server_addr));
    local_server_addr.sin_family = AF_INET;
    local_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_server_addr.sin_port = htons(UDP_TRANS_PORT);
    /* 绑定自己的端口和IP信息到socket上 */
    if(-1 == bind(sockfd, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)))
    {
        printf("bind error (func bind)\r\n");
        return(-2);
    }


    return sockfd;
}


/*
*监听TCP传输端口
*等待tcp客户端连接
*成功accept一个tcp客户端后
*需要创建相应的一条处理线程去处理
*参数为server端的TCP传输sockfd
*/
void recv_tcp_connect(void * param)
{
    int server_sockfd = *((int *)param);
    while(true)
    {
        /* 用于接收客户端连接 */
        int client_sockfd;
        struct sockaddr_in client_addr;
        memset(&client_addr,0,sizeof(client_addr));
        int addr_len = sizeof(client_addr);
        if(-1 == (client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len)))
        {
            printf("accept error (func accept)\r\n");
            return;
        }


        /* 成功accept一个tcp客户端，需要为每个tcp客户端创建一个处理消息线程 
        *设置分离态，使每条线程在退出后自动释放资源 
        */
        pthread_t tcp_receive_thread;
        pthread_create(&tcp_receive_thread, NULL, (void *)each_tcp_cli_recv, &client_sockfd);
        pthread_detach(tcp_receive_thread);
  }
}


/*
*监听UDP传输端口
*等待UDP客户端连接
*为每个UDP客户创建相应的一条处理线程去处理
*即每个客户对应一个端口
*参数为server端的UDP传输sockfd
*/
void recv_udp_connect(void * param)
{
    int server_sockfd = *((int *)param);

    while(true)
    {
        char tmp_msg[BUFLEN] = {0};//用于存客户端的消息，不做其他处理

        struct sockaddr_in client_addr, *tmp_client_addr;
        memset(&client_addr,0,sizeof(client_addr));
        int addr_len = sizeof(client_addr);

        int n;//用于获取recvfrom的返回值
        //接收到的消息固定为 udp client come
        n = recvfrom(server_sockfd, tmp_msg, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);
        if(-1 == n)
        {
            printf("recvfrom error (func recvfrom)\r\n");
            return;
        }

        tmp_client_addr = malloc(sizeof(struct sockaddr_in)); //线程传结构体需要分配到堆上
        *tmp_client_addr = client_addr;

        /* 设置分离态，使每条线程在退出后自动释放资源 */
        pthread_t udp_receive_thread;
        pthread_create(&udp_receive_thread, NULL, (void *)each_udp_cli_recv, tmp_client_addr);
        pthread_detach(udp_receive_thread);
    }
}


/*
*每成功accept一个tcp客户端
*需要为该tcp客户端创建一个下面的处理线程
*参数为该tcp客户端的sockfd
*/
void each_tcp_cli_recv(void * param)
{
    int client_sockfd = *((int *)param);
    char buf[BUFLEN + 1] = {0};
    int recv_size;
    while(true)
    {
        recv_size = recv(client_sockfd, buf, BUFLEN, 0);
        /* 0也是错误状态 客户端断开连接 */
        if(recv_size < 0 || 0 == recv_size)
        {
            /* 关闭socket 退出线程 由于已设置分离态，退出即释放资源 */
            close(client_sockfd);//关闭socket避免close_wait状态过多
            pthread_exit(0);
            return;
        }
        buf[recv_size] = '\0';
        deal_tcp_recv_message(client_sockfd,buf);
    }
}


/*
*需要为每个udp客户端创建一个下面的处理线程
*参数为该udp客户端的sockaddr_in结构体
*/
void each_udp_cli_recv(void * param)
{
    struct sockaddr_in client_addr = *((struct sockaddr_in *)param);
    free(param);
    
    /* 创建新的sockfd
    *为每个UDP客户端分配新的需要去连接的端口 
    */
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in new_server_addr;
    memset(&new_server_addr, 0, sizeof(new_server_addr));
    new_server_addr.sin_family = AF_INET;
    new_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    new_server_addr.sin_port = htons(++UDP_TRANS_PORT);
    if(-1 == bind(sockfd, (struct sockaddr*)&new_server_addr, sizeof(new_server_addr)))
    {
        printf("bind error (func bind)\r\n");
        return;
    }


    //通过这个新的sockfd向客户端发消息
    char new_server_msg[BUFLEN] = "New Server Here";

    int n;//记录sendto的返回值
    n = sendto(sockfd, new_server_msg, strlen(new_server_msg), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
    if(-1 == n)
    {
        printf("sendto error (func sendto)\r\n");
        close(sockfd);
        return;
    }

    //等待客户端回传消息并进行处理
    char tmp_msg[BUFLEN] = {0};
    int addr_len = sizeof(client_addr);
    while(true)
    {
        n = recvfrom(sockfd, tmp_msg, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);
        if(-1 == n)
        {
            printf("recvfrom error (func recvfrom)\r\n");
            close(sockfd);
            return;
        }
        deal_udp_recv_message(sockfd, client_addr, tmp_msg);
    }
}


/*
*对收到的tcp消息进行处理
*根据message类型做处理
*/
void deal_tcp_recv_message(int sockfd, char* str)
{
    /* 对于上传消息  分割str; m_list里存着msgtype，filename，filesize, hashcode 
    *  对于下载消息  分割str; m_list里存着msgtype，filename，fileset, hashcode 
    */
    char seg[] = ",";
    char m_list[4][BUFLEN];
    int i = 0;
    char *substr = strtok(str,seg);
    while(substr != NULL)
    {
        strcpy(m_list[i],substr);
        ++i;
        substr = strtok(NULL,seg);
    }

    //处理消息
    switch(atoi(m_list[0]))
    {
        case Message_Tcp_Upload:
        {
            tcp_upload_func(sockfd, m_list);
            break;
        }
        case Message_Tcp_Download:
        {
            tcp_download_func(sockfd, m_list);
            break;
        }
        default:
            break;
    }
}


/*
*对收到的udp消息进行处理
*根据message类型做处理
*/
void deal_udp_recv_message(int sockfd, struct sockaddr_in client_addr, char * str)
{
    /* 对于上传消息  分割str; m_list里存着msgtype，filename，filesize, hashcode 
    *  对于下载消息  分割str; m_list里存着msgtype，filename
    */
    char seg[] = ",";
    char m_list[4][BUFLEN];
    int i = 0;
    char *substr = strtok(str,seg);
    while(substr != NULL)
    {
        strcpy(m_list[i],substr);
        ++i;
        substr = strtok(NULL,seg);
    }

    //处理消息
    switch(atoi(m_list[0]))
    {
        case Message_Udp_Upload:
        {
            udp_upload_func(sockfd, client_addr, m_list);
            break;
        }
        case Message_Udp_Download:
        {
            udp_download_func(sockfd, client_addr, m_list);
            break;
        }
        default:
            break;
    }
}



/* 对TCP上传消息的处理函数 */
void tcp_upload_func(int sockfd, char msg[][BUFLEN])
{
    int flag = 0;//默认无需续传
    int b_size = 0;//用于fseek

    //先找是否存在断点文件
    char b_file[BUFLEN];
    sprintf(b_file, "%s.%s.bot.break", g_place_of_file, msg[1]);
    char tmp_zero[BUFLEN];//用于向客户端发送
    strcpy(tmp_zero, "0");
    if((access(b_file,F_OK)) != -1)//文件存在
    {
        FILE *fp_b;
        fp_b = fopen(b_file,"r");
        char tmp_size[BUFLEN];
        char tmp_hashcode[BUFLEN];
        fgets(tmp_size, BUFLEN, fp_b);
        tmp_size[strlen(tmp_size) -1] = '\0';//去掉\r\n
        fgets(tmp_hashcode, BUFLEN, fp_b);
        fclose(fp_b);
        if(0 == strcmp(tmp_hashcode, msg[3]))//文件名和hash值相同才需要续传
        {
            flag = 1;
            b_size = atoi(tmp_size);
            send(sockfd, tmp_size, BUFLEN, 0);
            remove(b_file);
        }
        else//同名直接覆盖
        {
            send(sockfd, tmp_zero, BUFLEN, 0);
            remove(b_file);
        }
    }
    else//文件不存在
    {
        send(sockfd, tmp_zero, BUFLEN, 0);
    }


    FILE *fp;
    char save_file[BUFLEN];//存储路径+文件名
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);
    if(0 == flag)//无需续传
    {
        fp = fopen(save_file, "wb+");//二进制方式打开 擦除源文件
    }
    else
    {
        fp = fopen(save_file, "rb+");//二进制方式打开 不擦除源文件
    }
    int dwCount = 0;//用于统计总字节数
    fseek(fp, b_size, SEEK_SET);//跳转到指定位置
    while(true)
    {
        char buf[BUFLEN] = {0};
        int length = recv(sockfd, (char *)buf, BUFLEN, 0);
        if(0 == length)//客户端断开
        {
            fclose(fp);

            //创建断点文件 格式为.filename.bot.break 隐藏，对外界面不可见
            FILE *fp_break;
            char break_file[BUFLEN];
            sprintf(break_file, "%s.%s.bot.break", g_place_of_file, msg[1]);
            //对同名文件的断点续传 只处理后面的续传，前面的被后面的同名文件覆盖了
            fp_break = fopen(break_file, "w+");
            char str_dwCount[BUFLEN];
            sprintf(str_dwCount, "%d", dwCount);
            fwrite(str_dwCount, 1, strlen(str_dwCount), fp_break);//dwcount
            fwrite("\r\n", 1, 2, fp_break);//换行
            fwrite(msg[3], 1, strlen(msg[3]), fp_break);//hashcode
            fclose(fp_break);

            //send 对于客户端断开的情况，这里就不写send了，反正客户端收不到

            break;
        }

        fwrite(buf, 1, length, fp);

        dwCount += length;
        if(dwCount == (atoi(msg[2]) - b_size))
        {
            /* 写入字节数相同,但可能存在包被篡改情况,需校验hashcode */
            fclose(fp);
            char HashCode[41];
            char response_msg[BUFLEN];
            strcpy(HashCode, get_sha1_from_file(save_file));
            if(0 == strcmp(HashCode, msg[3]))
            {
                if(0 == flag)//非续传返回
                {
                    sprintf(response_msg,"File Complete True; Sha1: %s", HashCode);
                }
                else//续传返回
                {
                    sprintf(response_msg,"Continue File Complete True; Sha1: %s", HashCode);
                }
                send(sockfd, response_msg, BUFLEN, 0);
            }
            else
            {
                if(0 == flag)//非续传返回
                {
                    sprintf(response_msg,"File Complete False; Sha1: %s", HashCode);
                }
                else//续传返回
                {
                    sprintf(response_msg,"Continue File Complete False; Sha1: %s", HashCode);
                }
                send(sockfd, response_msg, BUFLEN, 0);
            }
            break;
        }
    }
}


/* 对UDP上传消息的处理函数 */
void udp_upload_func(int sockfd, struct sockaddr_in client_addr, char msg[][BUFLEN])
{

    char tmp_response[BUFLEN] = "response";

    FILE *fp;
    char save_file[BUFLEN];//存储路径+文件名
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);
    fp = fopen(save_file, "wb+");//二进制方式打开 擦除源文件
    int dwCount = 0;//用于统计总字节数
    int addr_len = sizeof(client_addr);

    while(true)
    {
        char buf[BUFLEN] = {0};
        int length = recvfrom(sockfd, (char *)buf, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);

        //如果客户端发送传输完成
        if(0 == strcmp(buf, "OK"))
        {
            if(dwCount == atoi(msg[2]))
            {
                /* 写入字节数相同,但可能存在包被篡改情况,需校验hashcode */
                fclose(fp);
                char HashCode[41];
                char response_msg[BUFLEN];
                strcpy(HashCode, get_sha1_from_file(save_file));
                if(0 == strcmp(HashCode, msg[3]))
                {           
                    sprintf(response_msg,"File Complete True; Sha1: %s", HashCode);
                    sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                }
                else
                {
                    sprintf(response_msg,"File Complete False; Sha1: %s", HashCode);
                    sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                }
                break;
            }
        }

        fwrite(buf, 1, length, fp);
        //写入完成，发送应答包给客户端
        sendto(sockfd, tmp_response, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        dwCount += length;  
    }
}


/* 对TCP下载消息的处理函数 */
void tcp_download_func(int sockfd, char msg[][BUFLEN])
{
    int file_set = atoi(msg[2]);//从该位置继续发送
    char save_file[BUFLEN] = {0}; //文件的真实存放位置
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);
    char response_msg[BUFLEN] = {0};
    strcpy(response_msg, "0,0,0");//当文件不存在时的默认值，(flag,hash,size)
    if((-1 == access(save_file, F_OK)))//文件不存在
    {
        send(sockfd, response_msg, BUFLEN, 0);
    }
    else
    {
        struct stat s_buf;
        stat(save_file, &s_buf);//获取资源信息
        if(S_ISDIR(s_buf.st_mode))//如果是dir
        {
           send(sockfd, response_msg, BUFLEN, 0);
           return;
        }

        FILE *fp;
        char HashCode[41];//计算hash
        strcpy(HashCode, get_sha1_from_file(save_file));
        if(0 == strcmp(HashCode,msg[3]))
        {
            //需要续传
            sprintf(response_msg, "%d,%s,%d", 2, HashCode, get_file_size(save_file));
            send(sockfd, response_msg, BUFLEN, 0);
        }
        else
        {
            //不需要续传
            sprintf(response_msg, "%d,%s,%d", 1, HashCode, get_file_size(save_file));
            send(sockfd, response_msg, BUFLEN, 0);
            file_set = 0;//针对覆盖的情况
        }

        fp = fopen(save_file,"rb");
        char buf[BUFLEN + 1] = {0};
        fseek(fp, file_set, SEEK_SET);//从客户端传来的开始处开始
        int nLen = 0;//读取长度
        int nSize = 0;//发送长度
        int sdCount = 0;//记录发送了多少
        while(true)
        {
            nLen = fread(buf, 1, BUFLEN, fp);
            if(nLen == 0)
            {
                break;
            }
            nSize = send(sockfd, (const char *)buf, nLen, MSG_NOSIGNAL);
            /*当客户端断开，服务端向一个失效的socket send数据时
            *底层抛出SIGPIPE信号，该信号缺省处理方式为使服务端直接退出进程
            *flag设为MSG_NOSIGNAL忽略该信号
            */
            if(-1 == nSize)//发送出错,客户端断开
            {
                /* 服务端无法判断客户端实际接收了多少，不能做断点文件
                * 下载的续传功能只能在客户端做
                 */
                printf("A Client Disconnect\r\n");
                fclose(fp);
                return;
            }
            sdCount += nSize;
        }
        printf("\033[1;32mSend complete! buf size = %dbytes\r\n\033[0m", sdCount);
        fclose(fp);
    }
}


/* 对UDP下载消息的处理函数 */
void udp_download_func(int sockfd, struct sockaddr_in client_addr, char msg[][BUFLEN])
{
    char save_file[BUFLEN] = {0}; //文件的真实存放位置
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);

    char response_msg[BUFLEN] = {0};
    strcpy(response_msg, "0,0,0");//当文件不存在时的默认值，(flag,hash,size)
    if((-1 == access(save_file, F_OK)))//文件不存在
    {
        sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
    }
    else
    {
        struct stat s_buf;
        stat(save_file, &s_buf);//获取资源信息
        if(S_ISDIR(s_buf.st_mode))//如果是dir
        {
           sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
           return;
        }

        FILE *fp;
        char HashCode[41];//计算hash
        strcpy(HashCode, get_sha1_from_file(save_file));

        memset(response_msg,0,sizeof(response_msg));
        sprintf(response_msg, "%d,%s,%d", 1, HashCode, get_file_size(save_file));
        sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        

        fp = fopen(save_file,"rb");
        char buf[BUFLEN + 1] = {0};

        int nLen = 0;//读取长度
        int nSize = 0;//发送长度
        int sdCount = 0;//记录发送了多少
        int addr_len = sizeof(client_addr);
        while(true)
        {
            nLen = fread(buf, 1, BUFLEN, fp);
            if(nLen == 0)
            {
                break;
            }

            nSize = sendto(sockfd, (const char *)buf, nLen, MSG_NOSIGNAL, (struct sockaddr*)&client_addr, sizeof(client_addr));

            //等待应答
            recvfrom(sockfd, (char *)buf, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);
            sdCount += nSize;
        }

        //发送传输完成
        sendto(sockfd, "OK", 2, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        printf("\033[1;32mSend complete! buf size = %dbytes\r\n\033[0m", sdCount);
        fclose(fp);
    }
}