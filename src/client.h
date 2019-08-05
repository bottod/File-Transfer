#ifndef CLIENT_H
#define CLIENT_H



/*  最大系统命令个数为8个 */
#define MAX_COMAND_NUM 8

#define MCAST_ADDR "224.0.0.101"                    /* 一个局部连接多播地址，路由器不进行转发 */
#define MCAST_DATA "scan request"                   /* 多播发送的数据 */
#define MCAST_PORT 19971                            /* 多播端口 端口需要保证客户端和服务端一致 */
#define TCP_TRANS_PORT 19977                        /* TCP传输端口 端口需要保证客户端和服务端一致 */
#define UDP_TRANS_PORT 19977                        /* UDP传输端口 端口需要保证客户端和服务端一致 */
#define BUFLEN 1024                                 /* buf长度 */

/*定义上传的消息格式
*格式为
*  " Message_Type, FileName, FileSize, HashCode "
*/
enum Message_Type
{
	Message_Tcp_Ls,
	Message_Tcp_Download,
	Message_Tcp_Upload,
	Message_Udp_Download,
	Message_Udp_Upload
};


/* 定义一些简单的系统命令 */
enum Sys_Command
{
	Sys_Help,
	Sys_Connect,
	Sys_Download,
	Sys_Upload,
	Sys_Scan,
	Sys_Clear,
	Sys_Quit,
	Sys_Ls
};


/*
*文件信息结构体
*/
struct FileInfo
{
	char FileName[256];
	unsigned int FileSize; 
	char HashCode[41];
};

/* MAX_COMAND_NUM个系统命令 help, connect, download, upload, scan，clear, quit, ls */
char SysCommand[MAX_COMAND_NUM][20];
/*输入最多3个参数，每个参数19个字符以内，大小写无关 */
char cmd[3][20];

/* 显示cmd前置 */
void cmd_pre();

/* 初始化需要与Sys_Command同顺序 */
void init_sys_command();

/*解析输入的命令*/
int analysis(char* str);

/* 帮助菜单信息打印 */
void helpmenu();

/* 打印服务端目录，显示可下载的文件 */
void ls_func();



/* 获取文件大小信息 */
int get_file_size(char* filename);

/* 无回显读取字符 */
int getch();

/* 从路径中分离出单个文件名 */
char *get_file_single_name(char *filefullname);

/* 创建文件存放路径 */
void mk_place_of_file(char * file_dir);





//传输相关 

/*初始化一个tcp socket
*参数为用户输入的可用服务端的ip地址
*返回值为创建的该socket，用于赋值给g_client_tcp_sockfd 
*/
int client_tcp_sockfd_init(char* ip_addr);

/*初始化一个udp socket
*参数为用户输入的可用服务端的ip地址
*返回值为创建的该socket，用于赋值给g_client_udp_sockfd 
*/
int client_udp_sockfd_init(char* ip_addr);

/* udp组播，用于客户端探测服务端 */
int udp_scan();

/* 输出udp探测结果的消息 位于子线程中 */
void print_udp_scan_result(void * param);

/* 客户端连接服务端 包括tcp和udp */
void client_connect(char mcmd[][20]);

/* 客户端通过tcp向服务器上传数据 */
int client_tcp_upload(char* filename);

/* 客户端通过udp向服务端上传数据 */
int client_udp_upload(char* filename);

/* 客户端上传， tcp udp 对case里的包装 */
void client_upload_pack(char mcmd[][20]);

/* 客户端通过tcp从服务器下载数据 */
int client_tcp_download(char* filename);

/* 客户端通过udp从服务器下载数据 */
int client_udp_download(char* filename);

/* 客户端下载， tcp udp 对case里的包装 */
void client_download_pack(char mcmd[][20]);


#endif


