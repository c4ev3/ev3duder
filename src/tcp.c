/**
 * @file tcp-win.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief BSD Sockets/Winsock2 I/O wrappers
 */

#ifdef __WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Ws2tcpip.h>

#define bailout(msg) fprintf(stderr, "%s. Error Code : %d\n", msg, WSAGetLastError())
#define SHUT_RDWR SD_BOTH

static inline const char * inet_ntop(int af, const void * restrict src, char * restrict dst, socklen_t size);
typedef int socklen_t;

#else /* POSIX */

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define bailout(msg) perror(msg)
#define closesocket close

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
/**
 * \param [in] serial Serial-Number of Ev3. `NULL` connects to the first available
 * \return &fd pointer to file descriptor for use with tcp_{read,write,close,error}
 * \brief connects to a ev3 device on the same subnet.
 *
 * The Ev3 broadcast a UDP-packet every 5 seconds. This functions waits 7 seconds
 * for that to happen. It then compares the broadcasted serial number with the
 * value set should serial be non-`NULL`. If serial is `NULL` or the serial numbers
 * match, a udp packet is sent to the source port on the ev3.
 * Afterwards the Ev3 starts listening on the port it broadcasted.  
 * Then a GET request is sent and the VM starts listening and business is as usual
 * \see http://www.monobrick.dk/guides/how-to-establish-a-wifi-connection-with-the-ev3-brick/
 */ 

void *tcp_open(const char *serial)
{
#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		fprintf(stderr, "Failed to initialize Winsock2. Error Code : %d\n", WSAGetLastError());
		return NULL;
	}
#endif

	struct tcp_handle *fdp = malloc(sizeof *fdp);
	SOCKET fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		bailout("Failed to create socket.");
		return NULL;
	}

	struct sockaddr_in servaddr, cliaddr;
	memset(&servaddr, 0, sizeof servaddr);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port = htons(UDP_PORT);

	if (bind(fd, (struct sockaddr *)&servaddr, sizeof servaddr) == SOCKET_ERROR)
	{
		closesocket(fd);
		bailout("Failed to bind");
		return NULL;
	}

	socklen_t len = sizeof cliaddr;
	char buffer[128];
	ssize_t n;

	do {
		n = recvfrom(fd, buffer,1000,0,(struct sockaddr *)&cliaddr,&len);
		printf("n eqals = %d\n", n);

		if (n == SOCKET_ERROR)
		{
			closesocket(fd);
			bailout("Failed to recieve broadcast");
			return NULL;
		}

		buffer[n] = '\0';
		printf(">>%s<<\n", buffer);
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

	fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof servaddr);
	// FIXME: use designated initializers, use getpeername, use C99 (gustedt)
	// TODO: couldn't I use cliaddr directly?!
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = cliaddr.sin_addr.s_addr;

	inet_ntop(AF_INET, &cliaddr.sin_addr, fdp->ip, sizeof fdp->ip);
	servaddr.sin_port = htons(fdp->tcp_port);
	connect(fd, (struct sockaddr *)&servaddr, sizeof servaddr);
	if (n == SOCKET_ERROR)
	{
		closesocket(fd);
		bailout("Failed to initiate TCP connection");
		return NULL;
	}
	n = snprintf(buffer, sizeof buffer, 
			"GET /target?sn=%s VMTP1.0\nProtocol: %s",
			fdp->serial, fdp->protocol);
	printf("buffer: %s\n", buffer);

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
	printf("buffer=%s\n", buffer);

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
	union { u32 addr; u8 sub[4];} ip = {(struct in_addr*)src->s_addr};
	snprintf(dst, size, "%u.%u.%u.%u", 
			ip.sub[0], ip.sub[1], ip.sub[2], ip.sub[3]);
}
#endif
 
