// "CHATTING MANAGER"
// THEN THIS IS THE SERVER.

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

#define MULTICAST_ADDR "239.0.100.1"    // 니걸로 갱신해라

void error_handling(char *message)
{
    perror(message);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
	{
		printf("Usage : %s <PORT>\n", argv[0]);
		exit(1);
	}

    char buf[1024];
    int state;
    int port;

    // TIMEVAL for HEARTBEAT TIMEOUT.
    struct timeval tv;

    fd_set readfds;
    fd_set allfds;  // while(1) 내에서 매번 readfds 초기화하는 대신, allfds로 복사해서 요 복사본만 계속 수정되도록 함

    int maxfd;

    // hb_sock | msend.c
    int hb_sock;                    // HEARTBEAT 위한 [UDP] 소켓
    int time_live = TTL;
    struct sockaddr_in hbs_addr;    // heartbeatserver_address

    // serv_sock | uecho_server.c
    int serv_sock;                  // MESSAGING 위한 [TCP] 소켓 [굳이 이렇게 구분할거라면야...]
    struct ip_mreq join_addr;       // multicast_address
    struct sockaddr_in serv_addr;   // server_address

    //// INIT server sockets

    hb_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (hb_sock == -1) error_handling("UDP socket() error");

    serv_sock = socket(PF_INET, SOCK_STREAM, 0)
    if (serv_sock == -1) error_handling("TCP socket() error");

    port = atoi(argv[2]);

    // ip, port 정보 보내는 소켓
    send_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&adr, 0, sizeof(adr));
    sadr.sin_family = AF_INET;
    sadr.sin_addr.s_addr = inet_addr("239.0.100.1");
    sadr.sin_port = htons(port);

    setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));

    // JOIN MULTICAST GROUP
    join_addr.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
    join_addr.imr_interface.s_addr = htonl(INADDR_ANY);

    //// SET server options

    int on = 0;
    setsockopt(hb_sock, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on));    // 루프백 비활성화
    setsockopt(hb_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));   // TTL 설정

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(port+1);

    int state;
    
    state = bind(hb_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (state == -1) error_handling("bind() error");

    printf("OPENED UDP SOCKET.\n\n");

    state = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (state == -1) error_handling("TCP bind() error");

    state = listen(serv_sock, 5);
    if (state == -1) perror_exit("TCP listen() error");

    //// LAUNCH server.

    printf("OPENED TCP SOCKET.\n\n");

    printf("STARTED SERVER.\n");
    printf("SERVER IP: ");
    fflush(stdout);
    system("hostname -I");
    printf("Port: %d\n\n\n", atoi(argv[1]));
    fflush(stdout);

    // maxfd는 관찰하고 싶은 FD들 중 가장 큰 값을 가지고 있는 것으로 설정한다.
    maxfd = serv_sock;

    //// 변화 관찰하고 싶은 애들 등록.

    FD_ZERO(&readfds);
    FD_SET(serv_sock, &readfds);

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    while (1)
    {
        allfds = readfds;

        state = select(maxfd + 1, &allfds, NULL, NULL, &tv);

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
        }
    }
}
