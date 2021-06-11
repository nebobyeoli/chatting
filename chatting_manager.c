#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define MAX_BUF 1024
#define MAX_SOCK 50
#define TTL 5           // TIME TO LIVE

#define CMD_SIZE 4
#define NAME_SIZE 30
#define MSG_SIZE 1024

int client[MAX_SOCK];

void error_handling(char *message)
{
    perror(message);
    exit(0);
}

int main(int argc, char **argv)
{
    // 서버 프로그램 실행할 때 포트 명시해 주어야 함
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    /*************
    // yerinyerin
    int m_sockfd;  //multicast socket
    int hb_sockfd; //heartbeat socket

    *************/

    int server_sockfd, client_sockfd, sockfd;
    int state, client_len;
    int i, max_client, maxfd;

    // setsockopt() 사용하기 위한 int 변수에로의 할당
    int ttl = TTL;

    struct sockaddr_in clientaddr, serveraddr;  // client, server IP
    struct timeval tv;                          // 타임아웃 시간 길이
    fd_set readfds, otherfds, allfds;           // 클라이언트 소켓들의 변화 관찰

    char buf[MAX_BUF];  // 여기에 입력 받음
    int on = 1;

    state = 0;

    struct timeval tr;

	tr.tv_sec = 5;
	tr.tv_usec = 0;

    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error : ");
        exit(0);
    }

    memset(&serveraddr, 0x00, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);     // 서버 IP 등록
    serveraddr.sin_port = htons(atoi(argv[1]));         // 서버 포트 등록

    /*
     | SO_REUSEADDR:
     | 커널이 소켓의 포트를 점유 중인 상태에서도
     | 서버 프로그램을 다시 구동할 수 있도록 한다
     * 
     * 즉 서버를 닫고 다시 열었을 때, 사용했던 포트를 재사용할 수 있도록 한다.
     * 
     * 보통 서버 포트를 사용하고, 서버를 재시작했을 때 같은 포트를 사용하려 하면
     * 아래와 같은 문구가 뜬다
     * bind() error: Address already in use
     */
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

    // TIME TO LIVE 제한 걸기
    setsockopt(server_sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    state = bind(server_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (state == -1) { perror("bind error : "); exit(0); }

    state = listen(server_sockfd, 5);
    if (state == -1) { perror("listen error : "); exit(0); }

    client_sockfd = server_sockfd;

    // init
    max_client = -1;
    maxfd = server_sockfd;

    // 클라이언트 배열 초기화 하는 코드가
    // 여기에 있었네
    // 인덱스는 0부터 시작하기 때문에
    // [값이 설정되지 않음]을 -1로 표시해 주는 듯.
    for (i = 0; i < MAX_SOCK; i++) client[i] = -1;

    FD_ZERO(&readfds);               //readfds reset

    // readfds에 서버 소켓 추가.
    FD_SET(server_sockfd, &readfds); //server socket open

    // stashed
    /*
    FD_SET(m_sockfd, &readfds);   //multicast socket open
    FD_SET(hb_sockfd, &readfds); //heartbeat socket open
    */

    printf("\nTCP Server Starting ... \n\n\n");
    fflush(stdout);

    while (1)
    {
        // 
        // 기능 추가는 여기서 해 주면 될 것이다
        // 가장 바깥의 while (1) 반복문의 제일 윗부분
        // 
        // 클라이언트로부터의 수신을 먼저 필요로 하지 않으면
        // 여기에 짜라~
        // 
        // heartbeat 같은 경우에도
        // 서버가 먼저 보내는 거면
        // 저 아래 [// 모든 클라이언트에 UDP 메시지 전송]이라고 되어 있는 걸
        // 요 위로 올려서 적절-히 수정하면
        // 될 듯
        // 
        // n초에 한 번씩 보내려면 #include <time.h> 추가해서
        // time(0) 이용한 현재 시간과의 비교를 활용하면 될 거시다
        // 
        // hbimp_init_example.png를 참고하라!
        // 응가 화이팅!
        // 

        // readfds backup
        // 처럼 작동하는 것 같다.
        // 코드 보니까 아마도 소켓 확인은 allfds에서, 소켓 추가는 readfds에서 수행되는 듯.
        allfds = readfds;

        // 서버 소켓에 들어오는(요청되는, write 또는 접속 등의) 변화 관찰.
        /* 
         | man select
         | https://man7.org/linux/man-pages/man2/select.2.html
         | 
         | On success, select() and pselect() return the number of file descriptors
         | contained in the three returned descriptor sets
         | (that is, the total number of bits that are set in readfds, writefds, exceptfds).
         * 
         * select()와 pselect()는 인자로 받은 readfds에 포함되어 있는
         * FD의 수를 반환한다.
         * 
         * The return value may be zero
         * if the timeout expired before any file descriptors became ready.
         * 
         * select()는 둘째, 셋째, 넷째 인자로 넣은
         * &readfds, &writefds, &errorfds의 각각의 변화를 관찰하여,
         * 변화가 생긴 것이 있거나
         * 마지막 인자로 넣은 timeout이 끝날 때까지
         * 기다린다.
         * 
         * 아래와 같이 timeout에 NULL이 들어가면
         * 대기 없이 다음 단계로 바로 넘어가고(NON-BLOCKING),
         * 
         * struct timeval로 선언된 timeout 변수가 들어가면
         * timeout이 끝날 때까지(BLOCKING) select 내에서 반복문이 돌아간다.
         */
        /* 
         * 첫 번째 인자에 maxfd + 1이 들어가는 것은
         * 일단 man select에 명시된 조건에 따르면
         * 
         | nfds:
         | This argument should be set to the highest-numbered file descriptor
         | in any of the three sets, plus 1.
         | The indicated file descriptors in each set are checked,
         | up to this limit (but see BUGS).
         V 
         * 으로, 사용하는 FD 중 최댓값에 + 1을 더한 값을 넣어 주어야 한다고 나와 있다
         * 
         * 그리고 인덱스는 0부터 시작하고 개수는 1부터 시작하기 때문도 있지 않을까 싶다.
         */
        /*
         * 그래서 결론은
         * allfds(= readfds)에 포함된 소켓들의 변화를
         * 해당 순간에만 즉시 확인하여(timeout 인자 = NULL(0))
         * 변화가 발생한 fds(fd 세트)에 포함된 FD의 개수를
         * state에 저장하는 것이다
         */

        state = select(maxfd + 1, &allfds, NULL, NULL, &tr);

        // Server Socket - accept from client
        // 클라이언트 연결
        if (FD_ISSET(server_sockfd, &allfds))
        {
            client_len = sizeof(clientaddr);

            // 이때 클라이언트 소켓을 받아온다.
            client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len);
            
            /* inet_ntoa(clientaddr.sin_addr): 클라이언트 IP를 char 배열 형식으로 반환
             * ntohs(clientaddr.sin_port)    : 클라이언트 포트 or 프로세스 번호를 정수형으로 반환
             *
             * 예시: 같은 컴퓨터 다른 터미널에서 접속하면 아래와 같이 뜬다.
             * Connection from (10.0.2.4, 46896)
             */
            printf("\nconnection from (%s , %d)", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

            for (i = 0; i < MAX_SOCK; i++)
            {
                // 클라이언트 번호 설정이 안된곳 찾음
                if (client[i] < 0) 
                {
                    client[i] = client_sockfd;
                    printf("\nclientNUM=%d\nclientFD=%d\n", i + 1, client_sockfd);
                    break;
                }
            }

            // 할당된 클라이언트 # 번호 띄움
            printf("accept [%d]\n", client_sockfd);
            
            printf("===================================\n");

            // 관리할 수 있는 최대 클라이언트 수 도달/초과
            if (i == MAX_SOCK) perror("too many clients\n");

            FD_SET(client_sockfd, &readfds);

            // 위 반복되는 select()에서 사용되는
            // maxfd(클라이언트 #의 최댓값)의 값 갱신
            if (client_sockfd > maxfd) maxfd = client_sockfd;

            // 현재 클라이언트 수 변수(max_client) 갱신
            if (i > max_client) max_client = i;

            /* 
             | select()와 pselect()는 인자로 받은 readfds에 포함되어 있는
             | FD의 수를 반환한다.
             *
             * 현재 클라이언트 전에 연결된 클라이언트가 존재하는지를 확인하는 듯(?)
             * continue 실행 시 아래에 있는 for문이 실행되지 않으므로..
             */
            if (--state <= 0) continue;
        }

        // client socket check
        // 클라이언트와 연결 여부 체크
        for (i = 0; i <= max_client; i++)
        {
            sockfd = client[i];
            if (sockfd < 0) continue;

            // allfds에 sockfd가 저장되어 있음
            // 즉, sockfd가 accepted된 클라이언트일 때
            if (FD_ISSET(sockfd, &allfds))
            {
                memset(buf, 0, MAX_BUF);

                // 메시지 수신 시도했는데 read된 데이터가 없음
                // 즉, 클라이언트가 보낸 메시지가 없거나
                // 해당 클라이언트가 연결 해제됨.
                if (read(sockfd, buf, MAX_BUF) <= 0)
                {
                    printf("Close sockfd : %d\n", sockfd);
                    printf("===================================\n");

                    // Disconnect from Client
                    close(sockfd);
                    FD_CLR(sockfd, &readfds);
                    client[i] = -1;
                }

                // 클라이언트에게서 수신한 메시지가 존재함
                else
                {
                    printf("[%d]RECV: %s\n", sockfd, buf);

                    // 모든 클라이언트에 수신받은 메시지 전송
                    // client[]의 각 칸은 각 클라이언트의 FD 포인터(소켓)를 저장해서 그냥 client[]에 루프 돌려도 됨.
                    for (int j = 0; j <= max_client; j++)
                    {
                        // 끊어진 클라이언트 칸이면 무시
                        if (client[j] == -1) continue;

                        write(client[j], buf, MAX_BUF);
                        printf("Sent to client[%d]\n", client[j]);
                    }

                    // 모든 클라이언트에 UDP 메시지 전송
                    for (int j = 0; j <= max_client; j++)
                    {
                        if (client[j] == -1) continue;
                        sendto(client[j], buf, MAX_BUF, 0, (struct sockaddr *)&clientaddr, client_len);
                        printf("Sent UDPMSG to client[%d]\n", client[j]);
                    }
                }

                if (--state <= 0) break;
            }
        }
    }
}
