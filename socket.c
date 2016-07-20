#include "socket.h"
#include "types.h"

#include <assert.h>
#include <string.h>

#ifndef linux
#include <Ws2tcpip.h>

void* bzero(void*  buf, size_t size)
{
	return memset(buf, 0, size);
}

int inet_aton(const char *string, struct in_addr *addr)
{
	return inet_pton(AF_INET, string, addr);
}
#endif

#define LISTENQ 20

status_t
socket_init()
{
#ifdef linux
	//noting to do
	return K_SUCCESS;
#else
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 2);

	// 0 is success
	return WSAStartup(wVersionRequested, &wsaData);
#endif
}

status_t
socket_destory()
{
#ifdef linux
	//noting to do
#else
	return WSACleanup();
#endif
	return K_SUCCESS;
}

errno_t
socket_geterr()
{
#ifdef linux
	return errno;
#else
	return WSAGetLastError();
#endif
}


int
setnonblocking(socket_t sockfd)
{
#ifdef linux
		int opts;
		opts = fcntl(sockfd, F_GETFL);
		assert(opts >= 0);

		opts |= O_NONBLOCK;
		opts = fcntl(sockfd, F_SETFL);
		assert(opts >= 0);
#else
	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#endif
	return K_SUCCESS;
}


socket_t
client_create(const char* hostname, int portnumber)
{
	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	client_addr.sin_port = htons(0);

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0)
	{
		int error = socket_geterr();
		printf("Create Socket Failed->%d!\n", error);
		return K_ERROR;
	}

	if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))
	{
		printf("Client Bind Port Failed!\n");
		return K_ERROR;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	if (inet_aton(hostname, &server_addr.sin_addr) == 0)
	{
		printf("Server IP Address Error!\n");
		return K_ERROR;
	}
	server_addr.sin_port = htons(portnumber);
	socklen_t server_addr_length = sizeof(server_addr);

	if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0)
	{
		printf("Can Not Connect To %d!\n", portnumber);
		return K_ERROR;
	}
	return client_socket;
}


socket_t
server_create(const char* hostname, int portnumber)
{
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	server_addr.sin_port = htons(portnumber);

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		printf("Create Socket Failed!");
		return K_ERROR;
	}

	if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
	{
		printf("Server Bind Port : %d Failed!", portnumber);
		return K_ERROR;
	}
	return sockfd;
}

status_t
socket_send(socket_t sockfd,const char* data,int data_len)
{
	int data_send = 0;
	while (data_send < data_len)
	{
		//TODO
		int dsend = send(sockfd, data + data_send, data_len - data_send, 0);
		if (dsend < 0){
			printf("Send Failed:%d!", socket_geterr());
			return K_ERROR;
		}
		data_send += dsend;
	}
	return K_SUCCESS;
}

#define BUF_SIZE 1024

char*
socket_recv(socket_t sockfd, int* len)
{
	int buf_len = BUF_SIZE;
	char buf[BUF_SIZE];
	*len = recv(sockfd, buf, buf_len, 0);
	if (*len < 0){
		printf("Recv Failed:%d!", socket_geterr());
		return NULL;
	}
	else if (*len == 0){
		return NULL;
	}
	char* rbuf = (char*)malloc(*len);
	memcpy(rbuf, buf, *len);
	return rbuf;
}

char*
socket_recv_all(socket_t sockfd, int* len,int max_len)
{
	char* cache = NULL;
	int   rlen;
	int   recv_len = 0;
	char* rbuf = NULL;
	while ((cache = socket_recv(sockfd, &rlen)) != NULL){
		if (rbuf == NULL){
			rbuf = cache;
		}
		else{
			rbuf = (char*)realloc(rbuf, recv_len + rlen);
			memcpy(rbuf + recv_len, cache, rlen);
			socket_free(cache);
		}

		recv_len += rlen;
		if (recv_len >= max_len && max_len != -1)
			break;
		
	}
	*len = recv_len;
	return rbuf;
}


void
socket_free(char* data)
{
	free(data);
}


status_t
server_listen(socket_t serverfd)
{

	if (listen(serverfd, LISTENQ))
	{
		printf("Server Listen Failed!");
		return K_ERROR;
	}
	return K_SUCCESS;
}
