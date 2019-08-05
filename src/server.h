#ifndef SERVER_H
#define SERVER_H




#define BUFLEN 1024                     /* 本程序所有buf的长度都由此宏定义 */
#define UDP_MCAST_ADDR "224.0.0.101"    /* 多播地址 */
#define UDP_MCAST_PORT 19971            /* 多播端口 端口需要保证客户端和服务端一致 */
#define TCP_TRANS_PORT 19977            /* TCP传输端口 端口需要保证客户端和服务端一致 */

int UDP_TRANS_PORT = 19977;        		/* UDP传输端口 端口需要保证客户端和服务端一致 全局变量*/


/* 定义客户端发送的消息格式 
* 具体格式见deal_recv_message的定义处
*/
enum Message_Type
{
	Message_Tcp_Ls,
	Message_Tcp_Download,
	Message_Tcp_Upload,
	Message_Udp_Download,
	Message_Udp_Upload
};






/* 创建文件存放路径 */
void mk_place_of_file(char * file_dir);

/* 获取文件大小信息 */
int get_file_size(char* filename);






/*udp多播相关函数(见定义处)*/
int udp_mcast_init();

/* tcp传输socket初始化 */
int tcp_trans_init();

/* udp传输socket初始化 */
int udp_trans_init();

/* 等待TCP客户端到来的线程函数 */
void recv_tcp_connect(void * param);

/* 等待UDP客户端到来的线程函数 */
void recv_udp_connect(void * param);

/* 为每一个establish的tcp客户端处理消息的线程函数 */
void each_tcp_cli_recv(void * param);

/* 为每一个udp客户端处理消息的线程函数 */
void each_udp_cli_recv(void * param);

/* 对tcp收到的消息进行处理 */
void deal_tcp_recv_message(int sockfd, char * str);

/* 对udp收到的消息进行处理 */
void deal_udp_recv_message(int sockfd, struct sockaddr_in client_addr, char * str);

/* 对TCP上传消息的处理函数 */
void tcp_upload_func(int sockfd, char msg[][BUFLEN]);

/* 对UDP上传消息的处理函数 */
void udp_upload_func(int sockfd, struct sockaddr_in client_addr, char msg[][BUFLEN]);

/* 对TCP下载消息的处理函数 */
void tcp_download_func(int sockfd, char msg[][BUFLEN]);

/* 对UDP下载消息的处理函数 */
void udp_download_func(int sockfd, struct sockaddr_in client_addr, char msg[][BUFLEN]);

/* 显示目录内的文件发送给客户端 */
void ls_file(int sockfd);

#endif


