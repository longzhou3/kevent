#ifndef socket_h
#define socket_h

#include "types.h"

#include <errno.h>
#include <stdio.h>

#ifdef linux

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include<unistd.h>

typedef int                  socket_t;
typedef socklen_t            socklen_t;


#else

#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

typedef SOCKET               socket_t;
typedef int                  socklen_t;

#define close				 closesocket

#endif

#ifdef __cplusplus

extern "C" {

#endif

status_t
socket_init();

status_t
socket_destory();

errno_t
socket_geterr();

status_t
socket_send(socket_t sockfd, const char* data, int data_len);

char*
socket_recv(socket_t sockfd, int* len);

char*
socket_recv_all(socket_t sockfd, int* len, int max_len);

void
socket_free(char* data);

int
setnonblocking(socket_t sockfd);

socket_t
client_create(const char* hostname, int portnumber);

#ifdef __cplusplus

}

#endif
#endif
