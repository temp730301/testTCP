#include <pthread.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>

#define PORT_BASE		( 42222 )
#define MAIN_PORT		( 1 )
#define TCP_SND_PORT	( 2 )
#define TCP_RCV_POER	( 3 )

#define SOCKET_TYPE		( AF_INET )

void *udp_thread( void *param );
void *sndTcp_thread( void *param );
void *rcvTcpNotify_thread( void *param );

typedef struct {
	uint8_t msgId;
	uint8_t data[(1024*32)-1];
} msg_st;

int socketOpen( uint8_t port );
int socketSend( int sd, uint8_t port, uint8_t *data, uint32_t dataByte );

int socketOpen( uint8_t port )
{
	int result = socket( SOCKET_TYPE, SOCK_DGRAM, 0 );

	printf("socketOpen: socket: sd=%d err=%d\n", result, errno );

	struct sockaddr_in addr;
	(void)memset( &addr, 0, sizeof(addr) );
	addr.sin_family = SOCKET_TYPE;
	addr.sin_port = PORT_BASE + port;
	addr.sin_addr.s_addr = INADDR_ANY;
	int ret = bind( result, (struct sockaddr *)&addr, sizeof(addr) );

	printf("socketOpen: bind: ret=%d err=%d\n", ret, errno );

	return (result);
}

int socketSend( int sd, uint8_t port, uint8_t *data, uint32_t dataLen )
{
	struct sockaddr_in addr;
	(void)memset( &addr, 0, sizeof(addr) );
	addr.sin_family = SOCKET_TYPE;
	addr.sin_port = PORT_BASE + port;
	addr.sin_addr.s_addr = INADDR_ANY;
	
	int ret = sendto( sd, data, dataLen, 0, (struct sockaddr *)&addr, sizeof(addr) );
}

int socketReceive( int sd, uint32_t maxLen, uint8_t &port, uint8_t *data, uint32_t &receiveLen )
{
	socklen_t sin_size;
    struct sockaddr_in from_addr;
	
	recvfrom( sd, data, maxLen, 0, (struct sockaddr *)&from_addr, &sin_size);
	port = from_addr.sin_port;
	receiveLen = sin_size;

}

int main( void )
{
	int tcpSd = socket( SOCKET_TYPE, SOCK_STREAM, 0 );

	pthread_t pt1;
	pthread_create( &pt1, NULL, &sndTcp_thread, (void *)&tcpSd );
	pthread_t pt2;
	pthread_create( &pt2, NULL, &udp_thread, (void *)0 );
	pthread_t pt3;
	pthread_create( &pt3, NULL, &rcvTcpNotify_thread, (void *)&tcpSd );

	int mainSd = socketOpen( MAIN_PORT );

	while (1) {
		std::string menu;
		char inBuf[1024] = {0};
		std::cout << "input:" ;
		std::cin >> menu;
		uint8_t sndData[10] = { 0 };
		sndData[0] = 0x01;
		if (menu[0] == '1') {
			printf("menu = 1\n");
			int ret = socketSend( mainSd, TCP_SND_PORT, sndData, sizeof(sndData) );
			printf("menu1:ret=%d err=%d\n", ret, errno );
		}
		usleep(100000);
	}
	
	return( 0 );
}

void *sndTcp_thread( void *param )
{
	socklen_t sin_size;
    struct sockaddr_in from_addr;
    int sndTpcSocket = *((int *)param);
	printf("sndTcp: trhead started. param(sd)=%d\n", sndTpcSocket );

    unsigned char sendBuff[] = {
		0xb0, 0xc4, 0x00, 0x09, 0x00, 0x00, 0x00, 0x1e, 0x13, 0x43, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

	int msgSd = socketOpen( TCP_SND_PORT );

	uint8_t rcvMsg[1024*32] = { 0 };
	while (true) {
		uint32_t receiveLen;
		uint8_t port;
		socketReceive( msgSd, sizeof(rcvMsg), port, rcvMsg, receiveLen );
//		recvfrom( msgSd, rcvMsg, sizeof(rcvMsg), 0, (struct sockaddr *)&from_addr, &sin_size);
		if (0 < receiveLen) {
printf("rcv data=%02x\n", rcvMsg[0] );
			if (rcvMsg[0] == 0x01) {
				send( sndTpcSocket , sendBuff, sizeof(sendBuff), 0);
				printf( "send msg\n" );
			}
		}
	}
}

void *udp_thread( void *param )
{
	printf("---- start UDP thread\n");


 
    unsigned char udpsendBuff[] = {
		0xff, 0xff, 0x81, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x33, 0x01, 0x01, 0x02, 0x00,
		0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x06, 0x00, 0x00, 0x10, 0xb0, 0xc4, 0x00, 0x01,
		0x01, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x09, 0x04, 0x00,
		0xac, 0xa8, 0x00, 0x65, 0x00, 0x06, 0x77, 0x25 /*0xae, 0xbf*/
	}; 

    int udpSd = socket( SOCKET_TYPE, SOCK_DGRAM, 0 );

	printf( "udp: sd=%d err=%d\n", udpSd, errno );


	addrinfo hints;
	struct addrinfo *bind_address;
	getaddrinfo( "172.168.0.101", "30490", &hints, &bind_address );
 	int ret = bind( udpSd, bind_address->ai_addr, bind_address->ai_addrlen );
	freeaddrinfo( bind_address );

printf("udp: bind ret=%d errno=%d\n", ret, errno) ;
 
	struct sockaddr_in addr;
	addr.sin_family = SOCKET_TYPE;
    addr.sin_port = htons(30490);
    addr.sin_addr.s_addr = inet_addr("172.168.0.100");


	while (true) {
    	sendto( udpSd, udpsendBuff, sizeof(udpsendBuff), 0, (struct sockaddr *)&addr, sizeof(addr) );
//		printf(" udp: send udp data\n" );
		usleep( 1000000 );
 	}

}

void *rcvTcpNotify_thread( void *param )
{
	int notifySd = *((int *)param);

printf( "rcvNotify: socket.sd=%d err=%d\n", notifySd, errno );

	addrinfo		hints;
	struct addrinfo *bind_address;
	getaddrinfo( "172.168.0.101", "30501", &hints, &bind_address );
 	int ret = bind( notifySd, bind_address->ai_addr, bind_address->ai_addrlen );

printf( "rcvNotify: bind. ret=%d err=%d\n", ret, errno );

	freeaddrinfo( bind_address );

	struct sockaddr_in addr;
	addr.sin_family			= SOCKET_TYPE;
    addr.sin_port			= htons(30509);
    addr.sin_addr.s_addr	= inet_addr("172.168.0.100");

    ret = connect( notifySd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) );

printf( "rcvNotify: connect. ret=%d err=%d\n", ret, errno );

	char buf[1024*32] = { 0 };
	while (true) {
		int len = recv(notifySd, buf, sizeof(buf), 0);
		if (len > 4 ) {
			printf( "rcvNotify : len=%d data=%02x %02x %02x %02x %02x %02x %02x %02x \n", len,
				(unsigned char)buf[0], (unsigned char)buf[1], (unsigned char)buf[2], (unsigned char)buf[3],
				(unsigned char)buf[4], (unsigned char)buf[5], (unsigned char)buf[6], (unsigned char)buf[7] );
		}
	}
}