#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE	1024
#define	IDLE		0
#define	CHAT_SERVER_DOING	1
#define	CHAT_CLIENT_DOING	2

// My information Struct
// state 'e'가 뭔지는 모르겠으나
typedef struct {
	char name[20];	// chatting nickname
	char ip[20];	// my ip
	int	port;		// chatting TCP server port
	char statee;	// 0: IDLE   1:CHAT_SERVER_DOING  2:CHAT_CLIENT_DOING
} MyInfo;

int mcast_rsocket();
struct sockaddr_in addr;

int main(int argc, char *argv[])
{
	// common 
	MyInfo	myinfo;
	char buf[255];
	char msg[BUF_SIZE]={0, };
	
	// multicast
	int mcast_rcvsock;
	int str_len;
	int addr_size;
	char mbuf[BUF_SIZE];

	// hearbeat
	int hb_sock = -1;
	struct sockaddr_in hb_addr, from_addr;
	char ip[20] = { 0, }, cport[5] = {0,};
	int port;

	// tcp server and new client  for chat Server
	int tcp_servsock, new_clisock = -1, temp_clisock;
	struct sockaddr_in tcp_servaddr, cli_addr ;

	// tcp client for chat client
	int tcp_clisock;
	struct sockaddr_in tcp_cliaddr;
	int tcp_clisize = sizeof(tcp_cliaddr);

	// select
	fd_set 	allfds, readfds;
	int		fd, maxfd, state;

	if (argc != 3)
	{
		printf("Usage : %s <My IP> <Service PORT>\n", argv[0]);
		exit(1);
	}

	// keyboard input 
    fd = fileno(stdin);

	strcpy(myinfo.name , "CAPTAIN");
	strcpy(myinfo.ip, argv[1]);
	myinfo.port = atoi(argv[2]);
	myinfo.statee = IDLE;

	// for mcast receiver	
	mcast_rcvsock = mcast_rsocket();

	// for heartbeat 
    hb_sock = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&hb_addr, 0, sizeof(hb_addr));
    hb_addr.sin_family=AF_INET;

	// tcp chatting server socket
    tcp_servsock = socket(PF_INET, SOCK_STREAM, 0);
	
	// tcp chatting client socket
    tcp_clisock = socket(PF_INET, SOCK_STREAM, 0);
  
	maxfd = hb_sock; // max socket descriptor

	// 비워 줌
	FD_ZERO(&readfds);

	// 변화 관찰하고 싶은 애들 다 박아 놓음
	FD_SET(mcast_rcvsock,	&readfds);
	FD_SET(hb_sock,			&readfds);
	FD_SET(tcp_servsock, 	&readfds);
	FD_SET(tcp_clisock, 	&readfds);
	FD_SET(fd, 				&readfds);

	while (1)
	{
		allfds = readfds;
		
		memset(mbuf, 0, BUF_SIZE);
		state = select(maxfd + 1, &allfds, (fd_set *)0, (fd_set *)0, NULL);

		switch (state)
		{
			case -1: //error
			{
				exit(1);
			}

			case 0 : // timeout
			{
				exit(1);
				break;
			}

			default:
			{
				if (FD_ISSET(mcast_rcvsock, &allfds))
				{
					recvfrom(mcast_rcvsock, mbuf, BUF_SIZE, 0, (struct sockaddr*)&from_addr, &addr_size);

					// Get CM IP and Port from mbuf
					strcpy(ip, mbuf);
					memcpy(cport, &mbuf[20], 4);
					port = atoi(cport);
					printf("RCV mbuf> [%s] [%d]\n",ip, port);

					// Address and Port setting
					hb_addr.sin_addr.s_addr = inet_addr(ip);
					hb_addr.sin_port = htons(port);

					// send information to CM  
					memset(msg, 0, BUF_SIZE);
					memcpy(msg, myinfo.ip, sizeof(myinfo.ip));	
					sprintf(cport, "%d", myinfo.port); 
					memcpy(&msg[20], cport, 4); 
					msg[24] = myinfo.statee;
					
					sendto(hb_sock, msg, BUF_SIZE, 0, (struct sockaddr*)&hb_addr, sizeof(hb_addr));
				}

				if (FD_ISSET(tcp_servsock, &allfds))
				{
					temp_clisock = accept(tcp_servsock, (struct sockaddr *)&tcp_cliaddr, &tcp_clisize); // 클라이언트가 접속하면, accept 함수를 사용해 연결을 수립한다.
            		printf("Connection from (%s, %d)\n", inet_ntoa(tcp_cliaddr.sin_addr), ntohs(tcp_cliaddr.sin_port));

					if (myinfo.statee == IDLE)
					{
						// 클라이언트 가입 수락
						new_clisock = temp_clisock;
						if (maxfd < new_clisock) maxfd = new_clisock;
						FD_SET(new_clisock, &readfds);
						myinfo.statee = CHAT_SERVER_DOING;
					}
					else
					{
						// write(temp_clisock ); // send reject msg
						close(temp_clisock); 
					}
				}
				
				// RECEIVED NEW MESSAGE.
				// "recv when start Server chat"
				if (FD_ISSET(new_clisock, &allfds))
				{
					memset(msg, 0, BUF_SIZE);
					str_len = read(new_clisock, msg, BUF_SIZE);

					if (str_len <= 0)
					{
						FD_CLR(new_clisock, &readfds);
						close(new_clisock);
						new_clisock = -1;
						myinfo.statee = IDLE;
					}
					else
					{
						puts(msg);
					}
				}

				// "recv when start Client chat"
				if (FD_ISSET(tcp_clisock, &allfds))
				{
					memset(msg, 0, BUF_SIZE);
					str_len = read(tcp_clisock, msg, BUF_SIZE);
					
					if (str_len <= 0)
					{
						myinfo.statee = IDLE;
					}
					else
					{
						puts(msg);
					}
				}
// recv

// ================================================================ 뭐 표시를 이렇게 하겠다면야
// ================================================================

// send
				if (FD_ISSET(fd, &allfds))
				{
					memset(buf, 0, 255);
					fgets(buf, 255, stdin);

					// command mode
					if (strncmp(buf, "cmd", 3) == 0)
					{
						printf("CMD> %s",&buf[4]);
						
						// add command 

					}

					// chatting mode
					else if (strncmp(buf, "cht", 3) == 0)
					{
						printf("CHT> %s",&buf[4]);
						if ( myinfo.statee == CHAT_SERVER_DOING )
						{
							write(new_clisock, msg, BUF_SIZE);
						}
						else if ( myinfo.statee == CHAT_CLIENT_DOING )
						{
							write(tcp_clisock, msg, BUF_SIZE);
						}
					}
				}
				break;
			}
		}  // SWITCH
	}  // WHILE

	printf(" End Chat Program ======== \n");

	close(hb_sock);
	
	return 0;
}




// make Multicasting Receive Socket
int mcast_rsocket()
{
	int mcast_sock;
	struct ip_mreq join_addr;
	int on = 0;


	mcast_sock=socket(PF_INET, SOCK_DGRAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);	
	addr.sin_port=htons(5000);

	// Bind socket	
	if(bind(mcast_sock, (struct sockaddr*) &addr, sizeof(addr))==-1)
	{
		printf("bind() error");
		close(mcast_sock);
		exit(1);	
	}

	// Specify the multicast Group	
	join_addr.imr_multiaddr.s_addr=inet_addr("239.0.100.1");
	// Accept multicast from any interface
	join_addr.imr_interface.s_addr=htonl(INADDR_ANY);
  
	// Join Multicast Group	
	if ( (setsockopt(mcast_sock, IPPROTO_IP, 
		IP_ADD_MEMBERSHIP, (void*)&join_addr, sizeof(join_addr)))< 0 ) 
	{
		printf(" SetsockOpt Join Error \n");
		close(mcast_sock);
		exit(1);
	}

	return mcast_sock;
}
