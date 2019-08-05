/* printf���ڵ��̲߳��� */

#include <arpa/inet.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <dirent.h>

#include "server.h"/* .c�ļ�Ĭ�ϻ�Ѱ��ͬ����.h�ļ������Բ��� */
#include "./inc/sha1.h"

 

char g_place_of_file[BUFLEN];           /* ȫ�ֶ��������ļ���λ�� �������л�·�� */


int main(int argc, char **argv)
{

    /* Ĭ����ͬĿ¼�´���һ��server_file�ļ��� */
    //�ļ���ĩβһ��Ҫ��'/'
    //�ļ���ĩβһ��Ҫ��'/'
    //�ļ���ĩβһ��Ҫ��'/'
    strcpy(g_place_of_file, "./server_file/");
    mk_place_of_file(g_place_of_file);


    /* 
    *һ����������������ʱ�ȴ���3���̼߳����˿� 
    *�ֱ�Ϊ 
    *mcast_thread ����udp�㲥
    *tcp_trans_thread ����tcp����  ---->  �������Ӻ� tcp_receive_thread ���߳����ڴ�������
    *udp_trans_thread ����udp����
    */

    /* UDP�㲥�˿ڼ����߳� */
    pthread_t mcast_thread;
    pthread_create(&mcast_thread, NULL, (void *)udp_mcast_init, NULL);

    /* TCP����˿ڼ����߳� */
    int tcp_trans_sockfd = tcp_trans_init();
    if(tcp_trans_sockfd > 0)
    {
        pthread_t tcp_trans_thread;
        pthread_create(&tcp_trans_thread, NULL, (void *)recv_tcp_connect, &tcp_trans_sockfd);
    }

    /* UDP����˿ڼ����߳� */
    int udp_trans_sockfd = udp_trans_init();
    if(udp_trans_sockfd > 0)
    {
      pthread_t udp_trans_thread;
      pthread_create(&udp_trans_thread, NULL, (void *)recv_udp_connect, &udp_trans_sockfd);
    }


    /* ���߳� */
    char buf[1024];
    while(1)
    {
        scanf("%s",buf);
    }
}







/* �����ļ����·�� */
void mk_place_of_file(char * file_dir)
{
    if(NULL == opendir(file_dir))
    {
        mkdir(file_dir,0775);
    }
}


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






/*
*����˼���ಥ��
*�ಥ���ַ�ɺ궨��UDP_MCAST_ADDR����
*�ಥ��˿���UDP_MCAST_PORT����
*��������᷵�� int���� ������
*������Զѭ���ȴ�
*
*C:scan request
*S:scan response 200 OK
*/
int udp_mcast_init()
{
    /* ����socket ����UDP�鲥 ��Է�����̽�� */
    int sockfd; 
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        printf("socket creating error (func socket)\r\n");
        return(-1);
    }


    /* ��ʼ����ַ */
    struct sockaddr_in local_mcast_addr;
    memset(&local_mcast_addr, 0, sizeof(local_mcast_addr));
    local_mcast_addr.sin_family = AF_INET;
    local_mcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_mcast_addr.sin_port = htons(UDP_MCAST_PORT);
    /* ���Լ��Ķ˿ں�IP��Ϣ��socket�� */
    if(-1 == bind(sockfd, (struct sockaddr *) &local_mcast_addr, sizeof(local_mcast_addr)))
    {
        printf("bind error (func bind)\r\n");
        return(-2);
    }


    struct ip_mreq mreq;
    memset(&mreq, 0x00, sizeof(struct ip_mreq));
    /* �����鲥���ַ */
    mreq.imr_multiaddr.s_addr = inet_addr(UDP_MCAST_ADDR);
    /* ��������ӿڵ�ַ��Ϣ */
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    /* �����鲥��ַ*/
    if(-1 == setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)))
    {
        printf("add membership error (func setsockopt)\r\n");
        return(-3);
    }


    /* ��ֹ���ݻ��͵����ػػ��ӿ� */
    unsigned char loop = 0;
    if(-1 == setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,sizeof(loop)))
    {
        printf("set data loop error (func setsockopt)\r\n");
        return(-4);
    }
  

    /* ��¼�ͻ�����Ϣ */
    struct sockaddr_in client_addr;
    /* ���յ���Ϣbuf */
    char recv_msg[BUFLEN + 1];
    /* ����buf��ʵ�ʴ�С */
    unsigned int recv_size = 0;
    unsigned int addr_len = sizeof(struct sockaddr_in);
    /* ѭ�����������������鲥��Ϣ */
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
            /* �ɹ����յ����ݱ�,ĩβ�ӽ����� */
            recv_msg[recv_size] = '\0';

            /* ��ȡip��port */
            char ip_str[INET_ADDRSTRLEN];   /* INET_ADDRSTRLEN�����ϵͳĬ�϶��� 16 */
            inet_ntop(AF_INET,&(client_addr.sin_addr), ip_str, sizeof(ip_str));
            int port = ntohs(client_addr.sin_port);

            printf ("%s--%d : [%s]\r\n", ip_str, port, recv_msg);
            //��Է��ظ����ݣ���ʾ�յ�̽�⣬��������������Ա��������
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
*�����TCP����˿ڳ�ʼ��
*������ֵΪsockfd
*���ظ�ֵ�����
*listen���г����ɾֲ�����backlogָ��
*����sockfd������ⲿ���
*/
int tcp_trans_init()
{
    /* ����socket ����TCP���� */
    int sockfd; 
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        printf("socket creating error (func socket)\r\n");
        return(-1);
    }


    /* ��ʼ����ַ */
    struct sockaddr_in local_server_addr;
    memset(&local_server_addr, 0, sizeof(local_server_addr));
    local_server_addr.sin_family = AF_INET;
    local_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_server_addr.sin_port = htons(TCP_TRANS_PORT);
    /* ���Լ��Ķ˿ں�IP��Ϣ��socket�� */
    if(-1 == bind(sockfd, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)))
    {
        printf("bind error (func bind)\r\n");
        return(-2);
    }


    //ά�������ܺʹ�С10000(�ѽ�����δ�������׽���)
    int backlog = 10000;
    if(-1 == listen(sockfd,backlog))
    {
        printf("listen error (func listen)\r\n");
        return (-3);
    }
  

    return sockfd;
}


/*
*�����UDP����˿ڳ�ʼ��
*������ֵΪsockfd
*���ظ�ֵ�����
*����sockfd������ⲿ���
*/
int udp_trans_init()
{
    /* ����socket ����UDP���� */
    int sockfd; 
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        printf("socket creating error (func socket)\r\n");
        return(-1);
    }


    /* ��ʼ����ַ */
    struct sockaddr_in local_server_addr;
    memset(&local_server_addr, 0, sizeof(local_server_addr));
    local_server_addr.sin_family = AF_INET;
    local_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_server_addr.sin_port = htons(UDP_TRANS_PORT);
    /* ���Լ��Ķ˿ں�IP��Ϣ��socket�� */
    if(-1 == bind(sockfd, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)))
    {
        printf("bind error (func bind)\r\n");
        return(-2);
    }


    return sockfd;
}


/*
*����TCP����˿�
*�ȴ�tcp�ͻ�������
*�ɹ�acceptһ��tcp�ͻ��˺�
*��Ҫ������Ӧ��һ�������߳�ȥ����
*����Ϊserver�˵�TCP����sockfd
*/
void recv_tcp_connect(void * param)
{
    int server_sockfd = *((int *)param);
    while(true)
    {
        /* ���ڽ��տͻ������� */
        int client_sockfd;
        struct sockaddr_in client_addr;
        memset(&client_addr,0,sizeof(client_addr));
        int addr_len = sizeof(client_addr);
        if(-1 == (client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len)))
        {
            printf("accept error (func accept)\r\n");
            return;
        }


        /* �ɹ�acceptһ��tcp�ͻ��ˣ���ҪΪÿ��tcp�ͻ��˴���һ��������Ϣ�߳� 
        *���÷���̬��ʹÿ���߳����˳����Զ��ͷ���Դ 
        */
        pthread_t tcp_receive_thread;
        pthread_create(&tcp_receive_thread, NULL, (void *)each_tcp_cli_recv, &client_sockfd);
        pthread_detach(tcp_receive_thread);
  }
}


/*
*����UDP����˿�
*�ȴ�UDP�ͻ�������
*Ϊÿ��UDP�ͻ�������Ӧ��һ�������߳�ȥ����
*��ÿ���ͻ���Ӧһ���˿�
*����Ϊserver�˵�UDP����sockfd
*/
void recv_udp_connect(void * param)
{
    int server_sockfd = *((int *)param);

    while(true)
    {
        char tmp_msg[BUFLEN] = {0};//���ڴ�ͻ��˵���Ϣ��������������

        struct sockaddr_in client_addr, *tmp_client_addr;
        memset(&client_addr,0,sizeof(client_addr));
        int addr_len = sizeof(client_addr);

        int n;//���ڻ�ȡrecvfrom�ķ���ֵ
        //���յ�����Ϣ�̶�Ϊ udp client come
        n = recvfrom(server_sockfd, tmp_msg, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);
        if(-1 == n)
        {
            printf("recvfrom error (func recvfrom)\r\n");
            return;
        }

        tmp_client_addr = malloc(sizeof(struct sockaddr_in)); //�̴߳��ṹ����Ҫ���䵽����
        *tmp_client_addr = client_addr;

        /* ���÷���̬��ʹÿ���߳����˳����Զ��ͷ���Դ */
        pthread_t udp_receive_thread;
        pthread_create(&udp_receive_thread, NULL, (void *)each_udp_cli_recv, tmp_client_addr);
        pthread_detach(udp_receive_thread);
    }
}


/*
*ÿ�ɹ�acceptһ��tcp�ͻ���
*��ҪΪ��tcp�ͻ��˴���һ������Ĵ����߳�
*����Ϊ��tcp�ͻ��˵�sockfd
*/
void each_tcp_cli_recv(void * param)
{
    int client_sockfd = *((int *)param);
    char buf[BUFLEN + 1] = {0};
    int recv_size;
    while(true)
    {
        recv_size = recv(client_sockfd, buf, BUFLEN, 0);
        /* 0Ҳ�Ǵ���״̬ �ͻ��˶Ͽ����� */
        if(recv_size < 0 || 0 == recv_size)
        {
            /* �ر�socket �˳��߳� ���������÷���̬���˳����ͷ���Դ */
            close(client_sockfd);//�ر�socket����close_wait״̬����
            pthread_exit(0);
            return;
        }
        buf[recv_size] = '\0';
        deal_tcp_recv_message(client_sockfd,buf);
    }
}


/*
*��ҪΪÿ��udp�ͻ��˴���һ������Ĵ����߳�
*����Ϊ��udp�ͻ��˵�sockaddr_in�ṹ��
*/
void each_udp_cli_recv(void * param)
{
    struct sockaddr_in client_addr = *((struct sockaddr_in *)param);
    free(param);
    
    /* �����µ�sockfd
    *Ϊÿ��UDP�ͻ��˷����µ���Ҫȥ���ӵĶ˿� 
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


    //ͨ������µ�sockfd��ͻ��˷���Ϣ
    char new_server_msg[BUFLEN] = "New Server Here";

    int n;//��¼sendto�ķ���ֵ
    n = sendto(sockfd, new_server_msg, strlen(new_server_msg), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
    if(-1 == n)
    {
        printf("sendto error (func sendto)\r\n");
        close(sockfd);
        return;
    }

    //�ȴ��ͻ��˻ش���Ϣ�����д���
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
*���յ���tcp��Ϣ���д���
*����message����������
*/
void deal_tcp_recv_message(int sockfd, char* str)
{
    /* �����ϴ���Ϣ  �ָ�str; m_list�����msgtype��filename��filesize, hashcode 
    *  ����������Ϣ  �ָ�str; m_list�����msgtype��filename��fileset, hashcode 
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

    //������Ϣ
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
*���յ���udp��Ϣ���д���
*����message����������
*/
void deal_udp_recv_message(int sockfd, struct sockaddr_in client_addr, char * str)
{
    /* �����ϴ���Ϣ  �ָ�str; m_list�����msgtype��filename��filesize, hashcode 
    *  ����������Ϣ  �ָ�str; m_list�����msgtype��filename
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

    //������Ϣ
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



/* ��TCP�ϴ���Ϣ�Ĵ����� */
void tcp_upload_func(int sockfd, char msg[][BUFLEN])
{
    int flag = 0;//Ĭ����������
    int b_size = 0;//����fseek

    //�����Ƿ���ڶϵ��ļ�
    char b_file[BUFLEN];
    sprintf(b_file, "%s.%s.bot.break", g_place_of_file, msg[1]);
    char tmp_zero[BUFLEN];//������ͻ��˷���
    strcpy(tmp_zero, "0");
    if((access(b_file,F_OK)) != -1)//�ļ�����
    {
        FILE *fp_b;
        fp_b = fopen(b_file,"r");
        char tmp_size[BUFLEN];
        char tmp_hashcode[BUFLEN];
        fgets(tmp_size, BUFLEN, fp_b);
        tmp_size[strlen(tmp_size) -1] = '\0';//ȥ��\r\n
        fgets(tmp_hashcode, BUFLEN, fp_b);
        fclose(fp_b);
        if(0 == strcmp(tmp_hashcode, msg[3]))//�ļ�����hashֵ��ͬ����Ҫ����
        {
            flag = 1;
            b_size = atoi(tmp_size);
            send(sockfd, tmp_size, BUFLEN, 0);
            remove(b_file);
        }
        else//ͬ��ֱ�Ӹ���
        {
            send(sockfd, tmp_zero, BUFLEN, 0);
            remove(b_file);
        }
    }
    else//�ļ�������
    {
        send(sockfd, tmp_zero, BUFLEN, 0);
    }


    FILE *fp;
    char save_file[BUFLEN];//�洢·��+�ļ���
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);
    if(0 == flag)//��������
    {
        fp = fopen(save_file, "wb+");//�����Ʒ�ʽ�� ����Դ�ļ�
    }
    else
    {
        fp = fopen(save_file, "rb+");//�����Ʒ�ʽ�� ������Դ�ļ�
    }
    int dwCount = 0;//����ͳ�����ֽ���
    fseek(fp, b_size, SEEK_SET);//��ת��ָ��λ��
    while(true)
    {
        char buf[BUFLEN] = {0};
        int length = recv(sockfd, (char *)buf, BUFLEN, 0);
        if(0 == length)//�ͻ��˶Ͽ�
        {
            fclose(fp);

            //�����ϵ��ļ� ��ʽΪ.filename.bot.break ���أ�������治�ɼ�
            FILE *fp_break;
            char break_file[BUFLEN];
            sprintf(break_file, "%s.%s.bot.break", g_place_of_file, msg[1]);
            //��ͬ���ļ��Ķϵ����� ֻ��������������ǰ��ı������ͬ���ļ�������
            fp_break = fopen(break_file, "w+");
            char str_dwCount[BUFLEN];
            sprintf(str_dwCount, "%d", dwCount);
            fwrite(str_dwCount, 1, strlen(str_dwCount), fp_break);//dwcount
            fwrite("\r\n", 1, 2, fp_break);//����
            fwrite(msg[3], 1, strlen(msg[3]), fp_break);//hashcode
            fclose(fp_break);

            //send ���ڿͻ��˶Ͽ������������Ͳ�дsend�ˣ������ͻ����ղ���

            break;
        }

        fwrite(buf, 1, length, fp);

        dwCount += length;
        if(dwCount == (atoi(msg[2]) - b_size))
        {
            /* д���ֽ�����ͬ,�����ܴ��ڰ����۸����,��У��hashcode */
            fclose(fp);
            char HashCode[41];
            char response_msg[BUFLEN];
            strcpy(HashCode, get_sha1_from_file(save_file));
            if(0 == strcmp(HashCode, msg[3]))
            {
                if(0 == flag)//����������
                {
                    sprintf(response_msg,"File Complete True; Sha1: %s", HashCode);
                }
                else//��������
                {
                    sprintf(response_msg,"Continue File Complete True; Sha1: %s", HashCode);
                }
                send(sockfd, response_msg, BUFLEN, 0);
            }
            else
            {
                if(0 == flag)//����������
                {
                    sprintf(response_msg,"File Complete False; Sha1: %s", HashCode);
                }
                else//��������
                {
                    sprintf(response_msg,"Continue File Complete False; Sha1: %s", HashCode);
                }
                send(sockfd, response_msg, BUFLEN, 0);
            }
            break;
        }
    }
}


/* ��UDP�ϴ���Ϣ�Ĵ����� */
void udp_upload_func(int sockfd, struct sockaddr_in client_addr, char msg[][BUFLEN])
{

    char tmp_response[BUFLEN] = "response";

    FILE *fp;
    char save_file[BUFLEN];//�洢·��+�ļ���
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);
    fp = fopen(save_file, "wb+");//�����Ʒ�ʽ�� ����Դ�ļ�
    int dwCount = 0;//����ͳ�����ֽ���
    int addr_len = sizeof(client_addr);

    while(true)
    {
        char buf[BUFLEN] = {0};
        int length = recvfrom(sockfd, (char *)buf, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);

        //����ͻ��˷��ʹ������
        if(0 == strcmp(buf, "OK"))
        {
            if(dwCount == atoi(msg[2]))
            {
                /* д���ֽ�����ͬ,�����ܴ��ڰ����۸����,��У��hashcode */
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
        //д����ɣ�����Ӧ������ͻ���
        sendto(sockfd, tmp_response, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        dwCount += length;  
    }
}


/* ��TCP������Ϣ�Ĵ����� */
void tcp_download_func(int sockfd, char msg[][BUFLEN])
{
    int file_set = atoi(msg[2]);//�Ӹ�λ�ü�������
    char save_file[BUFLEN] = {0}; //�ļ�����ʵ���λ��
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);
    char response_msg[BUFLEN] = {0};
    strcpy(response_msg, "0,0,0");//���ļ�������ʱ��Ĭ��ֵ��(flag,hash,size)
    if((-1 == access(save_file, F_OK)))//�ļ�������
    {
        send(sockfd, response_msg, BUFLEN, 0);
    }
    else
    {
        struct stat s_buf;
        stat(save_file, &s_buf);//��ȡ��Դ��Ϣ
        if(S_ISDIR(s_buf.st_mode))//�����dir
        {
           send(sockfd, response_msg, BUFLEN, 0);
           return;
        }

        FILE *fp;
        char HashCode[41];//����hash
        strcpy(HashCode, get_sha1_from_file(save_file));
        if(0 == strcmp(HashCode,msg[3]))
        {
            //��Ҫ����
            sprintf(response_msg, "%d,%s,%d", 2, HashCode, get_file_size(save_file));
            send(sockfd, response_msg, BUFLEN, 0);
        }
        else
        {
            //����Ҫ����
            sprintf(response_msg, "%d,%s,%d", 1, HashCode, get_file_size(save_file));
            send(sockfd, response_msg, BUFLEN, 0);
            file_set = 0;//��Ը��ǵ����
        }

        fp = fopen(save_file,"rb");
        char buf[BUFLEN + 1] = {0};
        fseek(fp, file_set, SEEK_SET);//�ӿͻ��˴����Ŀ�ʼ����ʼ
        int nLen = 0;//��ȡ����
        int nSize = 0;//���ͳ���
        int sdCount = 0;//��¼�����˶���
        while(true)
        {
            nLen = fread(buf, 1, BUFLEN, fp);
            if(nLen == 0)
            {
                break;
            }
            nSize = send(sockfd, (const char *)buf, nLen, MSG_NOSIGNAL);
            /*���ͻ��˶Ͽ����������һ��ʧЧ��socket send����ʱ
            *�ײ��׳�SIGPIPE�źţ����ź�ȱʡ����ʽΪʹ�����ֱ���˳�����
            *flag��ΪMSG_NOSIGNAL���Ը��ź�
            */
            if(-1 == nSize)//���ͳ���,�ͻ��˶Ͽ�
            {
                /* ������޷��жϿͻ���ʵ�ʽ����˶��٣��������ϵ��ļ�
                * ���ص���������ֻ���ڿͻ�����
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


/* ��UDP������Ϣ�Ĵ����� */
void udp_download_func(int sockfd, struct sockaddr_in client_addr, char msg[][BUFLEN])
{
    char save_file[BUFLEN] = {0}; //�ļ�����ʵ���λ��
    sprintf(save_file, "%s%s", g_place_of_file, msg[1]);

    char response_msg[BUFLEN] = {0};
    strcpy(response_msg, "0,0,0");//���ļ�������ʱ��Ĭ��ֵ��(flag,hash,size)
    if((-1 == access(save_file, F_OK)))//�ļ�������
    {
        sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
    }
    else
    {
        struct stat s_buf;
        stat(save_file, &s_buf);//��ȡ��Դ��Ϣ
        if(S_ISDIR(s_buf.st_mode))//�����dir
        {
           sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
           return;
        }

        FILE *fp;
        char HashCode[41];//����hash
        strcpy(HashCode, get_sha1_from_file(save_file));

        memset(response_msg,0,sizeof(response_msg));
        sprintf(response_msg, "%d,%s,%d", 1, HashCode, get_file_size(save_file));
        sendto(sockfd, response_msg, BUFLEN, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        

        fp = fopen(save_file,"rb");
        char buf[BUFLEN + 1] = {0};

        int nLen = 0;//��ȡ����
        int nSize = 0;//���ͳ���
        int sdCount = 0;//��¼�����˶���
        int addr_len = sizeof(client_addr);
        while(true)
        {
            nLen = fread(buf, 1, BUFLEN, fp);
            if(nLen == 0)
            {
                break;
            }

            nSize = sendto(sockfd, (const char *)buf, nLen, MSG_NOSIGNAL, (struct sockaddr*)&client_addr, sizeof(client_addr));

            //�ȴ�Ӧ��
            recvfrom(sockfd, (char *)buf, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);
            sdCount += nSize;
        }

        //���ʹ������
        sendto(sockfd, "OK", 2, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        printf("\033[1;32mSend complete! buf size = %dbytes\r\n\033[0m", sdCount);
        fclose(fp);
    }
}