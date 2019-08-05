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

#include "client.h"/* .c�ļ�Ĭ�ϻ�Ѱ��ͬ����.h�ļ������Բ��� */
#include "./inc/sha1.h"
 


int g_client_tcp_sockfd = -1;                       /* ȫ��tcp socket  ��ֵΪsockfd������ */
int g_client_udp_sockfd = -1;                       /* ȫ��udp socket  ��ֵΪsockfd������ */
struct sockaddr_in g_udp_sockaddr;                  /* ��ȡȫ�ֵ�UDP�·���˴���˿ڣ� �Ǽ����˿�19977 */
char g_place_of_file[BUFLEN];                       /* ȫ�ֶ���ͻ����ļ�������λ�� �������л�·�� */






int main(int argc, char*argv)
{

    /* Ĭ����ͬĿ¼�´���һ��client_file�ļ����������� */
    //�ļ���ĩβһ��Ҫ��'/'
    //�ļ���ĩβһ��Ҫ��'/'
    //�ļ���ĩβһ��Ҫ��'/'
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
        //�������������
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


/* ��ʾcmdǰ�� */
void cmd_pre()
{
    printf("\033[1;32mBot@localhost:~\033[0m");
    printf("\033[0;34m$ \033[0m");
}


/* ��ʼ��SysCommand */
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


/* �����˵���Ϣ��ӡ */
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


/* �������������
*�����볬��3������������Ĳ�����������
*/
int analysis(char* str)
{
    //�������������3��������ÿ���������19���ַ�
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
        return -1;//�����룻
    }
     return -2;//�������룻
}





//���ߺ���
/* ��ȡ�ļ��Ĵ�С��Ϣ 
* ����ֵΪ�ļ���С
*/
int get_file_size(char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    int size=statbuf.st_size;
 
    return size;
}


/* �޻��Զ�ȡ�ַ� */
int getch(void)
{
    struct termios tm, tm_old;
    int fd = 0, ch;
    /* �������ڵ��ն����� */
    if(tcgetattr(fd, &tm) < 0) 
    {
        return -1;
    }
    tm_old = tm;
    /* �����ն�����Ϊԭʼģʽ����ģʽ�����е������������ֽ�Ϊ��λ������ */
    cfmakeraw(&tm);
    /* �����ϸ���֮������� */
    if(tcsetattr(fd, TCSANOW, &tm) < 0) 
    {
        return -1;
    }
 
    ch = getchar();

    /* ��������Ϊ��������� */
    if(tcsetattr(fd, TCSANOW, &tm_old) < 0) 
    {
        return -1;
    }
    
    return ch;
}


/* ��·���з���������ļ��� */
char *get_file_single_name(char *filefullname)
{
    int x = strlen(filefullname);
    char ch = '/';
    char *filesiglename = strrchr(filefullname,ch) + 1;

    return filesiglename;
}


/* �����ļ����·�� */
void mk_place_of_file(char * file_dir)
{
    if(NULL == opendir(file_dir))
    {
        mkdir(file_dir,0775);
    }
}





/*������غ���*/
/* udp�鲥�����ڿͻ���̽������ 
*���þֲ��׽��֣�scanһ�δ���һ���������ر��׽���
*C:scan request
*S:scan response 200 OK
*/
int udp_scan()
{
    printf("\033[1;32m~~ Press 'q' to jump out the scan module ~~\033[0m\r\n");
    //�ֲ��׽��֣��������ر�
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);         /*�����׽���*/
    if(-1 == sockfd)
    {
        printf("socket error (func socket)\r\n");
        return -1;
    }


    /* ��ʼ����ַ */
    struct sockaddr_in mcast_addr; 
    memset(&mcast_addr, 0, sizeof(mcast_addr));/*��ʼ��IP�ಥ��ַΪ0*/
    mcast_addr.sin_family = AF_INET;                /*����Э��������ΪAF*/
    mcast_addr.sin_addr.s_addr = inet_addr(MCAST_ADDR);/*���öಥIP��ַ*/
    mcast_addr.sin_port = htons(MCAST_PORT);        /*���öಥ�˿�*/

    /*��ಥ��ַ��������*/
    int send_len = sendto(sockfd, MCAST_DATA, sizeof(MCAST_DATA), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr)) ;
    if(send_len < 0)
    {
        printf ("sendto error (func sendto)\r\n");
        return -2;
    }

    /*���ͺ�ʼѭ�����ܷ����19971�˿ڷ�������Ϣ������ʲô��Ϣ����¼
    *����Ϣ��������������
    */
    pthread_t mcast_msg_thread;
    pthread_create(&mcast_msg_thread, NULL, (void *)print_udp_scan_result, &sockfd);
    pthread_detach(mcast_msg_thread);//�ر��̼߳��ͷ���Դ

    /* recvfrom �� getch ��Ϊ������������˾��������߳����ڽ�����ʾ
    *���߳����ڽ����û�����
    */
    while(true)
    {
        if(getch() == 'q')
        {
            /* �����ԣ���������̲߳��˳������ */
            pthread_cancel(mcast_msg_thread);//�����ź�ȡ���߳�
            return 0;
        }
    }
}


/* ���udp̽��������Ϣ λ�����߳��� */
void print_udp_scan_result(void * param)
{
    int sockfd = *((int *)param);
    /* ��¼���߷������Ϣ */
    struct sockaddr_in online_server_addr;
    /* ���յ���Ϣbuf */
    char recv_msg[BUFLEN + 1];
    /* ����buf��ʵ�ʴ�С */
    unsigned int recv_size = 0;
    unsigned int addr_len = sizeof(struct sockaddr_in);

    //��������ȡ�� recvfrom
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
        /* �ɹ����յ����ݱ�,ĩβ�ӽ����� */
            recv_msg[recv_size] = '\0';

            /* ��ȡip��port */
            char ip_str[INET_ADDRSTRLEN];   /* INET_ADDRSTRLEN�����ϵͳĬ�϶��� 16 */
            inet_ntop(AF_INET,&(online_server_addr.sin_addr), ip_str, sizeof(ip_str));
            int port = ntohs(online_server_addr.sin_port);

            printf("%s--%d : [%s]\r\n", ip_str, port, recv_msg);
        }
    }
}


/*��ʼ��һ��tcp socket
*����Ϊ�û�����Ŀ��÷���˵�ip��ַ
*����ֵΪ�����ĸ�socket�����ڸ�ֵ��g_client_tcp_sockfd 
*/
int client_tcp_sockfd_init(char* ip_addr)
{
    /* ����socket */
    int sockfd;
    if(-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        return -1;
    }

    /* ��ʼ����ַ */
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


/*��ʼ��һ��udp socket
*����Ϊ�û�����Ŀ��÷���˵�ip��ַ
*����ֵΪ�����ĸ�socket�����ڸ�ֵ��g_client_udp_sockfd 
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
    memset(first_msg, 0, sizeof(first_msg));//���first_msg
    //��ȡ�µķ���˵Ķ˿ڵ�ַ��Ϣ
    int n = recvfrom(sockfd, first_msg, BUFLEN, 0, (struct sockaddr *)&server_addr, &addr_len);//��ʱfirst_msg���ֵΪNew Server Here
    if(-1 == n)
    {
        //����ʧ��
        return -1;
    }
    /*�ɹ���ȡ���µķ���˵Ķ˿ڵ�ַ��Ϣ�󣬿��Խ��н�һ��������upload��download
    *������˵�server_addr���浽g_udp_sockaddr
    */
    g_udp_sockaddr = server_addr;//ǳ���������ǹ�����
    return sockfd;
}


/* �ͻ������ӷ���� ����tcp��udp 
* ����Ĳ���Ϊ�û������������ָ��
*/
void client_connect(char mcmd[][20])
{
    /* ���ȱ�ٵڶ������� */
    if(0 == strcasecmp(mcmd[1],""))
    {
        printf("\033[1;31mUsage: 'connect IpAddress'\r\n\033[0m");
    }
    else/* ���������ʽ��ȷ ���ж�IP���ж�û��ʵ��*/
    {
        //tcp
        close(g_client_tcp_sockfd);//�ȹر�socket
        if(0 > (g_client_tcp_sockfd = client_tcp_sockfd_init(mcmd[1]))) //sockfd��������
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
        if(0 > (g_client_udp_sockfd = client_udp_sockfd_init(mcmd[1]))) //sockfd��������
        {
            printf("\033[1;31mGet Udp Server Fail! Check if server online!\033[0m\r\n");
        }
        else
        {
            printf("\033[1;32mGet Udp Server Succeed! UDP action Can Use!\r\n\033[0m");
        }

    }
}


/* �ͻ����ϴ������ ����tcp��udp 
* ����Ĳ���Ϊ�û������������ָ��
*/
void client_upload_pack(char mcmd[][20])
{
    /* tcp ģʽ */
    if(0 == strcasecmp(mcmd[1], "tcp")) 
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename Ϊ��
        {
            printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
        }
        else//���������ʽ��ȷ
        {
            if(0 > g_client_tcp_sockfd)//���sockfd�Ƿ�����
            {
                printf("\033[1;31mPlease connect a server first!!!\r\n\033[0m");
            }
            else //tcp
            {
                if((access(mcmd[2],F_OK)) != -1)//�ж���Դ�Ƿ����
                {
                    struct stat s_buf;
                    stat(mcmd[2], &s_buf);//��ȡ��Դ��Ϣ
                    if(S_ISREG(s_buf.st_mode))//�ж���file����dir
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
    /* udp ģʽ */
    else if(0 == strcasecmp(mcmd[1],"udp"))
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename Ϊ��
        {
            printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
        }
        else//���������ʽ��ȷ
        {
            if(0 > g_client_udp_sockfd)//���sockfd�Ƿ�����
            {
                printf("\033[1;31mPlease Check If UDP Server online!!!\r\n\033[0m");
            }
            else //tcp
            {
                if((access(mcmd[2],F_OK)) != -1)//�ж���Դ�Ƿ����
                {
                    struct stat s_buf;
                    stat(mcmd[2], &s_buf);//��ȡ��Դ��Ϣ
                    if(S_ISREG(s_buf.st_mode))//�ж���file����dir
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
    /* ������� */
    else
    {
        printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
    }
}


/* tcpģʽ���ϴ��ļ��������� 
* socket ����g_client_tcp_sockfd
*�����ʹ��ǰ��Ҫע�� ��socket�Ƿ��Ѿ���ȷ��ʼ��
*��Ϣ��ʽΪ
*  " Message_Type, FileName, FileSize, HashCode "
*upload ���ļ��� �����Ǹ��ݱ���·����ѡ�� ��./dir/test.txt
*/
int client_tcp_upload(char* filename)
{
    /*���ļ�������ͬһ�ȸĳ�·�����͵ģ�������data��Ҫת��Ϊ./data
    *ϵͳ�����ļ������������ '/'
    */
    char new_filename[BUFLEN] = {0};//���ڴ洢�µ�·�������ļ���
    if(NULL == strchr(filename, '/'))//û�ҵ�
    {
        sprintf(new_filename,"./%s", filename);
    }
    else
    {
        strcpy(new_filename, filename);
    }

    /* �ļ���Ϣ�ṹ���ʼ�� */
    struct FileInfo m_fileinfo;
    memset(&m_fileinfo, 0, sizeof(m_fileinfo));
    strcpy(m_fileinfo.FileName,new_filename);
    m_fileinfo.FileSize = get_file_size(m_fileinfo.FileName);
    strcpy(m_fileinfo.HashCode, get_sha1_from_file(m_fileinfo.FileName));


    char str[BUFLEN] = {0};
    sprintf(str,"%d,%s,%d,%s", Message_Tcp_Upload, get_file_single_name(m_fileinfo.FileName), m_fileinfo.FileSize, m_fileinfo.HashCode);
    printf("Sha1 Code: %s\r\n", m_fileinfo.HashCode);

    //����Ҫ�ϴ����ļ���Ϣ
    send(g_client_tcp_sockfd, str, strlen(str), 0);

    //���շ���˷������Ƿ���ڶϵ��ļ���Ϣ
    recv(g_client_tcp_sockfd, str, BUFLEN, 0);//���͸�ʽΪ ��ʲôλ�ÿ�ʼ����intֵ,���ļ���Ϊ0���жϵ��ļ���Ϊ����0��ֵ
    int from_where = atoi(str);

    //ѡ���ǴӶϵ㴦�������·���
    FILE *fp;
    fp = fopen(m_fileinfo.FileName,"rb");
    if(fp == NULL)
    {
        return -3;
    }

    char buf[BUFLEN + 1] = {0};
    fseek(fp, from_where, SEEK_SET);//�ӷ���˴����Ŀ�ʼ����ʼ
    int nLen = 0;//��ȡ����
    int nSize = 0;//���ͳ���
    int sdCount = 0;//��¼�����˶���
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

    /* �ȴ�����˻ش���Ϣ ��֤�ļ����� */
    if(0 == recv(g_client_tcp_sockfd, (char *)buf, BUFLEN, 0))
    {
        //����˴��س���
        return 0;
    }
    printf("Server: %s\r\n", buf);

    return 0;
}


/* udpģʽ���ϴ��ļ��������� 
* socket ����g_client_udp_sockfd
*�����ʹ��ǰ��Ҫע�� ��socket�Ƿ��Ѿ���ȷ��ʼ��
*��Ϣ��ʽΪ
*  " Message_Type, FileName, FileSize, HashCode "
*upload ���ļ��� �����Ǹ��ݱ���·����ѡ�� ��./dir/test.txt
*/
int client_udp_upload(char* filename)
{
    /*���ļ�������ͬһ�ȸĳ�·�����͵ģ�������data��Ҫת��Ϊ./data
    *ϵͳ�����ļ������������ '/'
    */
    char new_filename[BUFLEN] = {0};//���ڴ洢�µ�·�������ļ���
    if(NULL == strchr(filename, '/'))//û�ҵ�
    {
        sprintf(new_filename,"./%s", filename);
    }
    else
    {
        strcpy(new_filename, filename);
    }

    /* �ļ���Ϣ�ṹ���ʼ�� */
    struct FileInfo m_fileinfo;
    memset(&m_fileinfo, 0, sizeof(m_fileinfo));
    strcpy(m_fileinfo.FileName,new_filename);
    m_fileinfo.FileSize = get_file_size(m_fileinfo.FileName);
    strcpy(m_fileinfo.HashCode, get_sha1_from_file(m_fileinfo.FileName));


    char str[BUFLEN] = {0};
    sprintf(str,"%d,%s,%d,%s", Message_Udp_Upload, get_file_single_name(m_fileinfo.FileName), m_fileinfo.FileSize, m_fileinfo.HashCode);
    printf("Sha1 Code: %s\r\n", m_fileinfo.HashCode);

    //����Ҫ�ϴ����ļ���Ϣ
    sendto(g_client_udp_sockfd, str, strlen(str), 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));

    //���ļ�׼����ʼ�����ļ�
    FILE *fp;
    fp = fopen(m_fileinfo.FileName,"rb");
    if(fp == NULL)
    {
        return -3;
    }

    char buf[BUFLEN + 1] = {0};
    int nLen = 0;//��ȡ����
    int nSize = 0;//���ͳ���
    int sdCount = 0;//��¼�����˶���
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
        
        //�ȴ������Ӧ��
        recvfrom(g_client_udp_sockfd, buf, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len);
        
        sdCount += nSize;
        printf("%d of %d has sended!\r\n", sdCount, m_fileinfo.FileSize);
    }
    //���ʹ��������Ϣ
    sendto(g_client_udp_sockfd, "OK", 2, 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));

    printf("\033[1;32mSend complete! buf size = %dbytes\r\n\033[0m", sdCount);
    fclose(fp);

    /* �ȴ�����˻ش���Ϣ ��֤�ļ����� */
    memset(str, 0, sizeof(buf));
    if(0 == recvfrom(g_client_udp_sockfd, buf, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len))
    {
        //����˴��س���
        return 0;
    }
    printf("Server: %s\r\n", buf);

    return 0;
}


/* �ͻ��˴ӷ�������� ����tcp��udp 
* ����Ĳ���Ϊ�û������������ָ��
*/
void client_download_pack(char mcmd[][20])
{
    /* tcp ģʽ */
    if(0 == strcasecmp(mcmd[1], "tcp")) 
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename Ϊ��
        {
            printf("\033[1;31mUsage: 'download TCP/UDP filename'\r\n\033[0m");
        }
        else//���������ʽ��ȷ
        {
            if(0 > g_client_tcp_sockfd)//���sockfd�Ƿ�����
            {
                printf("\033[1;31mPlease connect a server first!!!\r\n\033[0m");
            }
            else //tcp
            {
                client_tcp_download(mcmd[2]);
            }
        }
    }
    /* udp ģʽ */
    else if(0 == strcasecmp(mcmd[1],"udp"))
    {
        if(0 == strcasecmp(mcmd[2], ""))//filename Ϊ��
        {
            printf("\033[1;31mUsage: 'download TCP/UDP filename'\r\n\033[0m");
        }
        else//���������ʽ��ȷ
        {
            if(0 > g_client_tcp_sockfd)//���sockfd�Ƿ�����
            {
                printf("\033[1;31mPlease Check If UDP Server online!!!\r\n\033[0m");
            }
            else //tcp
            {
                client_udp_download(mcmd[2]);
            }
        }
    }
    /* ������� */
    else
    {
        printf("\033[1;31mUsage: 'upload TCP/UDP filename'\r\n\033[0m");
    }
}


/* tcpģʽ�´ӷ����������ļ�
* socket ����g_client_tcp_sockfd
*�����ʹ��ǰ��Ҫע�� ��socket�Ƿ��Ѿ���ȷ��ʼ��
*��Ϣ��ʽΪ
*  " Message_Type, FileName, FileSet, HashCode "
* FileSet һ��Ϊ0 ���жϵ��ļ�ʱΪsizeֵ
* HashCode һ��Ϊ0 ���жϵ��ļ�ʱΪhashcodeֵ
* �ϵ��ļ��ṹ�� size \r\n hashcode
* filename ֻ�����ǵ�һ���ļ����������԰���·������ֻ�����server��ָ�����ļ���������
*/
int client_tcp_download(char *filename)
{
    //�ͻ��˵��жϣ� �ڷ����Ҳ�����ж� 
    if(NULL != strchr(filename, '/'))//�����·�����͵��ļ���
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

    //�����Ƿ���ڶϵ��ļ�
    char b_file[BUFLEN];
    sprintf(b_file, "%s.%s.bot.break", g_place_of_file, filename);
    if((access(b_file,F_OK)) != -1)//�ļ�����
    {
        FILE *fp_b;
        fp_b = fopen(b_file,"r");
        fgets(tmp_size, BUFLEN, fp_b);
        tmp_size[strlen(tmp_size) -1] = '\0';//ȥ��\r\n
        fgets(tmp_hashcode, BUFLEN, fp_b);
        fclose(fp_b);
    }

    char str[BUFLEN] = {0};
    sprintf(str, "%d,%s,%d,%s", Message_Tcp_Download, filename, atoi(tmp_size), tmp_hashcode);
    //�����������������ˣ���ʽΪmsg_type, filename, fileset,hashcode
    send(g_client_tcp_sockfd, str, strlen(str), 0);
    //���շ���˻ش����ļ�������Ϣ����ʽΪ is_exist,hashcode,filesize  �Ե�һ������ 0:�ļ������� 1�������� 2������
    recv(g_client_tcp_sockfd, str, BUFLEN, 0);
    
    /* �ָ�str��Ϣ ����m_msg�� */
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

    if(0 == atoi(m_msg[0]))//������ļ�������
    {
        printf("Your Requested File \033[1;31mNot Exist!!!\r\n\033[0m");
        //���������ļ��������򲻹����޶ϵ��ļ�����ɾ���öϵ��ļ�
        return 0;
    }
    else//������ļ����ڣ���֤hashֵ����ͬ����������ͬ�򸲸ǣ����߶�Ҫɾ���ϵ��ļ���
    {
        //���������ļ������Ҹ��ļ��жϵ��ļ�����ɾ���öϵ��ļ�
        if(0 != strcmp(tmp_hashcode, "0"))//��֤�Ƿ���ڶϵ��ļ� 
        {
            remove(b_file);//�жϵ���ɾ��
        }

        FILE *fp;
        if(1 == atoi(m_msg[0]))//��������
        {
            fp = fopen(save_file, "wb+");//�����Ʒ�ʽ�� ����Դ�ļ�
            strcpy(tmp_size, "0");//����ļ�������Ҫ����Ϊ0
        }
        else//2 ����Ҫ����
        {
            fp = fopen(save_file, "rb+");//�����Ʒ�ʽ�� ������Դ�ļ�
        }


        int dwCount = 0;//����ͳ�����ֽ���
        fseek(fp, atoi(tmp_size), SEEK_SET);//��ת��ָ��λ��
        while(true)
        {
            char buf[BUFLEN] = {0};
            int length = recv(g_client_tcp_sockfd, (char *)buf, BUFLEN, 0);
            if(0 == length)//����˶Ͽ��������ϵ��ļ�
            {
                fclose(fp);

                //�����ϵ��ļ� ��ʽΪ.filename.bot.break ���أ�������治�ɼ�
                FILE *fp_break;
                //��ͬ���ļ��Ķϵ����� ֻ��������������ǰ��ı������ͬ���ļ�������
                fp_break = fopen(b_file, "w+");
                char str_dwCount[BUFLEN];
                sprintf(str_dwCount, "%d", dwCount);
                fwrite(str_dwCount, 1, strlen(str_dwCount), fp_break);//dwcount
                fwrite("\r\n", 1, 2, fp_break);//����
                fwrite(m_msg[1], 1, strlen(m_msg[1]), fp_break);//hashcode
                fclose(fp_break);

                break;
            }

            fwrite(buf, 1, length, fp);

            dwCount += length;
            printf("%d of %d has reveived!\r\n", dwCount, atoi(m_msg[2]) - atoi(tmp_size));
            if(dwCount == (atoi(m_msg[2]) - atoi(tmp_size)))
            {
                /* д���ֽ�����ͬ,�����ܴ��ڰ����۸����,��У��hashcode */
                fclose(fp);
                char HashCode[41];
                strcpy(HashCode, get_sha1_from_file(save_file));//�Լ�����д����ɵ��ļ���hashcode
                if(0 == strcmp(HashCode, m_msg[1]))
                {
                    if(1 == atoi(m_msg[0]))//����������
                    {
                        printf("File Complete True; Sha1: %s\r\n", HashCode);
                    }
                    else//��������
                    {
                        printf("Continue File Complete True; Sha1: %s\r\n", HashCode);
                    }
                }
                else
                {
                    if(1 == atoi(m_msg[0]))//����������
                    {
                        printf("File Complete False; Sha1: %s\r\n", HashCode);
                    }
                    else//��������
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


/* udpģʽ�´ӷ����������ļ�
* socket ����g_client_udp_sockfd
*�����ʹ��ǰ��Ҫע�� ��socket�Ƿ��Ѿ���ȷ��ʼ��
*��Ϣ��ʽΪ
*  " Message_Type, FileName "
* filename ֻ�����ǵ�һ���ļ����������԰���·������ֻ�����server��ָ�����ļ���������
*/
int client_udp_download(char *filename)
{
    //�ͻ��˵��жϣ� �ڷ����Ҳ�����ж� 
    if(NULL != strchr(filename, '/'))//�����·�����͵��ļ���
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

    //�����������������ˣ���ʽΪmsg_type, filename, fileset,hashcode
    sendto(g_client_udp_sockfd, str, strlen(str), 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));
    //���շ���˻ش����ļ�������Ϣ����ʽΪ is_exist,hashcode,filesize  �Ե�һ������ 0:�ļ������� 1������
    recvfrom(g_client_udp_sockfd, str, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len);
    
    /* �ָ�str��Ϣ ����m_msg�� */
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

    if(0 == atoi(m_msg[0]))//������ļ�������
    {
        printf("Your Requested File \033[1;31mNot Exist!!!\r\n\033[0m");
        //���������ļ��������򲻹����޶ϵ��ļ�����ɾ���öϵ��ļ�
        return 0;
    }
    else//������ļ�����
    {
        FILE *fp;
        fp = fopen(save_file, "wb+");//�����Ʒ�ʽ�� ����Դ�ļ�

        int dwCount = 0;//����ͳ�����ֽ���
        while(true)
        {
            char buf[BUFLEN] = {0};
            int length = recvfrom(g_client_udp_sockfd, (char *)buf, BUFLEN, 0, (struct sockaddr *)&g_udp_sockaddr, &addr_len);
            
            //����� �����ļ����
            if(0 == strcmp(buf, "OK"))
            {
                if(dwCount == atoi(m_msg[2]))
                {
                    /* д���ֽ�����ͬ,�����ܴ��ڰ����۸����,��У��hashcode */
                    fclose(fp);
                    char HashCode[41];
                    strcpy(HashCode, get_sha1_from_file(save_file));//�Լ�����д����ɵ��ļ���hashcode
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
            //д����� ������Ӧ��
            sendto(g_client_udp_sockfd, tmp_response, BUFLEN, 0, (struct sockaddr*)&g_udp_sockaddr, sizeof(g_udp_sockaddr));

            dwCount += length;
            printf("%d of %d has reveived!\r\n", dwCount, atoi(m_msg[2]));
        }
    }

    return 0;
}



/* ��ʾ����˿����ص��ļ� */
void ls_func()
{
    if(0 > g_client_tcp_sockfd)//���tcp sockfd�Ƿ�����
    {
        printf("\033[1;31mPlease connect a server first!!!\r\n\033[0m");
    }
    else //tcp
    {
        char str[2 * BUFLEN] = {0};
        sprintf(str, "%d,0", Message_Tcp_Ls);
        //����ls���������ˣ���ʽΪmsg_type, 0
        send(g_client_tcp_sockfd, str, strlen(str), 0);
        //���շ���˻ش����ļ�������Ϣ����ʽΪ is_exist,hashcode,filesize  �Ե�һ������ 0:�ļ������� 1�������� 2������
        recv(g_client_tcp_sockfd, str, 2 * BUFLEN, 0);
        printf("%s\r\n",str);
    }
}