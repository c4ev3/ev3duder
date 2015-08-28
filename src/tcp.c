/**
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief BSD Sockets/Winsock2 I/O wrappers
 */

#ifdef __WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Ws2tcpip.h>

#define bailout(msg) ({\
	char *buf; \
	int error = WSAGetLastError(); \
	if (FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,\
    NULL, error, 0, (char*)&buf, 0, NULL) && error) \
		fprintf(stderr, "%s. Error message: %s\n", (msg), buf);\
	else \
		fprintf(stderr, "%s. Error code: %d\n", (msg), error); \
	LocalFree(buf);\
	})
#define SHUT_RDWR SD_BOTH

static inline const char * inet_ntop(int af, const void * restrict src, char * restrict dst, socklen_t size);
static inline int inet_pton(int af, const char *src, void *dst);

typedef int socklen_t;

#define socksetblock(sock, y_n) \
    ioctlsocket((sock), FIONBIO, &(unsigned long){!(y_n)})

#else /* POSIX */

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define bailout(msg) perror((msg))
#define closesocket close
#define socksetblock(sock, y_n) ((y_n) ? \
	fcntl((sock), F_SETFL, fcntl((sock), F_GETFL, 0) & ~O_NONBLOCK) : \
    fcntl((sock), F_SETFL, O_NONBLOCK))

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include "btserial.h"

#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "tcp.h"

//! default UDP broadcast port
#define UDP_PORT 3015
#define TCP_PORT 5555

#define UDP_RECV_TIMEOUT 6
#define TCP_CONNECT_TIMEOUT 1
/**
 * \param [in] serial IP-Address or Serial-Number of Ev3. `NULL` connects to the first available
 * \return &fd pointer to file descriptor for use with tcp_{read,write,close,error}
 * \brief connects to a ev3 device on the same subnet.
 *
 * If the serial number contains a dot, a direct TCP connection is attempted.
 * The Ev3 broadcasts a UDP-packet every 5 seconds. This functions waits 7 seconds
 * for that to happen. It then compares the broadcasted serial number with the
 * value set should serial be non-`NULL`. If serial is `NULL` or the serial numbers
 * match, a udp packet is sent to the source port on the ev3.
 * Afterwards the Ev3 starts listening on the port it broadcasted.  
 * Then a GET request is sent and the VM starts listening and business is as usual
 * \see http://www.monobrick.dk/guides/how-to-establish-a-wifi-connection-with-the-ev3-brick/
 */ 

void *tcp_open(const char *serial, unsigned timeout)
{
#ifdef _WIN32
	DWORD tv_udp = 1000*(timeout  ?: UDP_RECV_TIMEOUT);
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		fprintf(stderr, "Failed to initialize Winsock2. Error Code : %d\n", WSAGetLastError());
		return NULL;
	}
#else
	struct timeval tv_udp = {.tv_sec = timeout ?: UDP_RECV_TIMEOUT};
#endif

	struct tcp_handle *fdp = malloc(sizeof *fdp);
	SOCKET fd;
	char buffer[128];
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof servaddr);
	ssize_t n;

	if (serial && strchr(serial, '.')) // we have got an ip
	{
		/*
		 * This only works because the LEGO firmware doesn't even parse the 
		 * submitted serialnumber. It just strstr(2)s for /target?sn and reports
		 * success if its found. This isn't guaranteed to stay, however
		 */
		servaddr.sin_family = AF_INET;
		switch(inet_pton(AF_INET, serial, &servaddr.sin_addr)){
			case 1: break;
			case 0: fprintf(stderr, "Invalid IP specified '%s'\n", serial); return NULL;
			case -1: bailout("Parsing IP failed"); return NULL;
		}
		strncpy(fdp->name, "[n/a]", sizeof fdp->name);
		strncpy(fdp->protocol, "[n/a]", sizeof fdp->protocol);
		strncpy(fdp->serial, "[n/a]", sizeof fdp->serial);
		servaddr.sin_port = htons(fdp->tcp_port = TCP_PORT);
		strncpy(fdp->ip, serial, sizeof fdp->ip);
		
	}else{ // we need to search for it
		if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
		{
			bailout("Failed to create socket");
			return NULL;
		}

		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
		servaddr.sin_port = htons(UDP_PORT);
		if (bind(fd, (struct sockaddr *)&servaddr, sizeof servaddr) == SOCKET_ERROR)
		{
			closesocket(fd);
			bailout("Failed to bind");
			return NULL;
		}

		struct sockaddr_in cliaddr;
		socklen_t len = sizeof cliaddr;
		do {
			setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv_udp, sizeof tv_udp);
			n = recvfrom(fd, buffer,sizeof buffer,0,(struct sockaddr *)&cliaddr,&len);
			if (n == SOCKET_ERROR)
			{
				closesocket(fd);
				bailout("Failed to recieve broadcast"); // normally means timeout
				return NULL;
			}

			buffer[n] = '\0';
			sscanf(buffer, 
					"Serial-Number: %s\r\n"
					"Port: %u\r\n"
					"Name: %s\r\n"
					"Protocol: %s\r\n",
					fdp->serial, &fdp->tcp_port, fdp->name, fdp->protocol);
		}while(serial && strcmp(serial, fdp->serial) != 0);

		n = sendto(fd, (char[]){0x00}, 1, 0, (struct sockaddr *)&cliaddr, sizeof cliaddr);
		closesocket(fd);
		if (n == SOCKET_ERROR)
		{
			bailout("Failed to initiate handshake");
			return NULL;
		}

		memset(&servaddr, 0, sizeof servaddr);
		// FIXME: use designated initializers, use getpeername, use C99 (gustedt)
		// TODO: couldn't I use cliaddr directly?!
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = cliaddr.sin_addr.s_addr;
		inet_ntop(AF_INET, &cliaddr.sin_addr, fdp->ip, sizeof fdp->ip);
		servaddr.sin_port = htons(fdp->tcp_port);
	}
		
	fd = socket(AF_INET, SOCK_STREAM, 0);

	socksetblock(fd, 0);

	n = connect(fd, (struct sockaddr *)&servaddr, sizeof servaddr);
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	struct timeval tv_tcp = {.tv_sec = timeout ?: TCP_CONNECT_TIMEOUT};
	if (select(fd + 1, NULL, &fdset, NULL, &tv_tcp) == 1)
	{
#ifndef _WIN32
		int so_error;
		socklen_t len = sizeof so_error;
		getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
		if (so_error == 0)
#endif
		   n = 0; 
	}
	socksetblock(fd, 1);

	if (n == SOCKET_ERROR)
	{
		closesocket(fd);
		bailout("Failed to initiate TCP connection");
		return NULL;
	}
	
	n = snprintf(buffer, sizeof buffer, 
			"GET /target?sn=%s VMTP1.0\nProtocol: %s",
			fdp->serial, fdp->protocol);
	n = sendto(fd, buffer, n, 0, (struct sockaddr *)&servaddr, sizeof servaddr);
	if (n == SOCKET_ERROR)
	{
		closesocket(fd);
		bailout("Failed to handshake over TCP");
		return NULL;
	}
	
	n=recvfrom(fd, buffer, sizeof buffer, 0, NULL, NULL);
	if (n == SOCKET_ERROR)
	{
		closesocket(fd);
		bailout("Failed to recieve TCP-connection confirmation"); 
		return NULL;
	}

	buffer[n] = '\0';
	//	printf("buffer=%s\n", buffer);
#ifdef _WIN32
	fdp->sock = (LPVOID)fd;
#else
	fdp->fd[0] = fdp->fd[1] = fd;
#endif
	return fdp;
}


/**
 * \param [in] device handle returned by tcp_open()
 * \brief releases the socket file descriptor opened by tcp_open()
 */
void tcp_close(void *sock)
{
	shutdown(*(SOCKET*)sock, SHUT_RDWR);
	closesocket(*(SOCKET*)sock);
	free(sock);
#ifdef _WIN32
	WSACleanup();
#endif
}
/**
 * \param [in] device handle returned by tcp_open()
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \bug it's useless. Could use \p wprintf and \p strerror
 */
const wchar_t *tcp_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}

#ifdef _WIN32
int tcp_write(void* sock, const u8* buf, size_t count) {
	buf++;count--;
	return send(*(SOCKET*)sock, (char*)buf, count, 0); 
}
int tcp_read(void* sock, u8* buf, size_t count, int milliseconds) {
	//FIXME: Timeout!
	(void) milliseconds;
	buf++;count--;
	return recv(*(SOCKET*)sock, (char*)buf, count, 0);
}
#else

int (*tcp_write)(void* device, const u8* buf, size_t count) = bt_write;
int (*tcp_read)(void* device, u8* buf, size_t count, int milliseconds) = bt_read;

#endif

#ifdef _WIN32
static inline const char *inet_ntop(int af, const void * restrict src, char * restrict dst, socklen_t size)
{
	assert(af == AF_INET);
	union { u32 addr; u8 sub[4];} ip = {((struct in_addr*)src)->s_addr};
	snprintf(dst, size, "%u.%u.%u.%u", 
			ip.sub[0], ip.sub[1], ip.sub[2], ip.sub[3]);
	return dst;
}
static inline int inet_pton(int af, const char *src, void *dst)
{
	assert(af == AF_INET);
	union { u32 addr; u8 sub[4];} ip;
	int n = 
	sscanf(src, "%hhu.%hhu.%hhu.%hhu", 
			&ip.sub[0], &ip.sub[1], &ip.sub[2], &ip.sub[3]);
	if (n != 4)
		return 0;
	
	((struct in_addr*)dst)->s_addr = ip.addr;
	return 1;
}

#endif
 
