#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>


#define TTL 2
#define BUF_SIZE 1024
#define IP_SIZE 20
#define PORT_SIZE 5

int main(int argc, char *argv[])
{
    char buf[1024];
    int state;
    int port;

    //timer 설정
    struct timeval tv;  
    tv.tv_sec = 5; 
    tv.tv_usec = 0;

    fd_set readfds;

    int maxfd;

    // send_sock      msend.c
    int send_sock, on = 1;
    int time_live = TTL;
    struct sockaddr_in adr, sadr;

    // serv_sock       uecho_server.c
    int serv_sock;
    struct sockaddr_in serv_adr;

    //ip, port 저장용
    char ip[IP_SIZE]={0,}, cport[PORT_SIZE]={0,};


    if (argc != 3)
	{
		printf("Usage : %s <IP> <PORT>\n", argv[0]);
		exit(1);
	}

    port = atoi(argv[2]);

    // ip, port 정보 보내는 소켓
    send_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&adr, 0, sizeof(adr));
    sadr.sin_family = AF_INET;
    sadr.sin_addr.s_addr = inet_addr("239.0.100.1");
    sadr.sin_port = htons(port);

    setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));



    //udp 소켓 설정
    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        printf("UDP socket creation error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(port+1);

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        printf("bind() error");

    FD_ZERO(&readfds);



    while (1)
    {
        FD_SET(serv_sock, &readfds);

        state = select(maxfd + 1, &readfds, (fd_set *)0, (fd_set *)0, &tv);

        switch (state)
        {
        case -1: //error
            perror("select error : ");
            exit(0);
            break;
        case 0:
            printf("Time over (3sec)\n");

            memset(buf, 0, sizeof(buf));
            strcpy(buf, argv[1]);
            sprintf(&buf[IP_SIZE], "%d", port+1);
            //udp 메시지 전송
            if (sendto(send_sock, buf, BUF_SIZE, 0, (struct sockaddr *)&sadr, sizeof(sadr)) != -1){
                printf("UDP 메시지 전송 성공\n");
            }

            tv.tv_sec = 3; 
            tv.tv_usec = 0;

            break;
        default:
            if (FD_ISSET(serv_sock, &readfds))
            {
                //하트비트 온거 받는다
                memset(buf, 0, sizeof(buf));
                recvfrom(serv_sock, buf, BUF_SIZE, 0,0, 0);
                printf("하트비트수신 : %s %s\n", buf, &buf[20]);
            }
            break;
        }
    }
}
