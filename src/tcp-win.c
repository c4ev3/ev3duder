/**
 * @file tcp-win.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Winsock2 I/O wrappers
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
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
	struct tcp_handle *fdp = malloc(sizeof *fdp);
	WSADATA wsa;
		printf("%s:%d!\n", __func__, __LINE__);

	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        fprintf(stderr, "Failed to initialize Winsock2. Error Code : %d\n", WSAGetLastError());
        return NULL;
    }
	printf("%s:%d!\n", __func__, __LINE__);
	
	SOCKET fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		fprintf(stderr, "Failed to create socket. Error Code : %d\n", WSAGetLastError());
		return NULL;
	}
		printf("%s:%d!\n", __func__, __LINE__);

	struct sockaddr_in servaddr, cliaddr;
	memset(&servaddr, 0, sizeof servaddr);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port = htons(UDP_PORT);
	printf("%s:%d!\n", __func__, __LINE__);

	if (bind(fd, (struct sockaddr *)&servaddr, sizeof servaddr) == SOCKET_ERROR)
	{
		closesocket(fd);
		perror("Failed to bind");
		return NULL;
	}
		printf("%s:%d!\n", __func__, __LINE__);

    int len = sizeof cliaddr;
	char buffer[128];
	ssize_t n;
	printf("%s:%d!\n", __func__, __LINE__);

	do {
			printf("%s:%d!\n", __func__, __LINE__);

		n = recvfrom(fd, buffer,1000,0,(struct sockaddr *)&cliaddr,&len);
		printf("n eqals = %d\n", n);
	printf("%s:%d!\n", __func__, __LINE__);

		if (n == SOCKET_ERROR)
		{
			closesocket(fd);
			fprintf(stderr, "Failed to recieve broadcast. Error Code : %d\n", WSAGetLastError());
			return NULL;
		}
	printf("%s:%d!\n", __func__, __LINE__);

		buffer[n] = '\0';
			printf("%s:%d!\n", __func__, __LINE__);
		printf(">>%s<<\n", buffer);
		sscanf(buffer, 
				"Serial-Number: %s\r\n"
				"Port: %u\r\n"
				"Name: %s\r\n"
				"Protocol: %s\r\n",
		fdp->serial, &fdp->tcp_port, fdp->name, fdp->protocol);
	printf("%s:%d!\n", __func__, __LINE__);

		}while(serial && strcmp(serial, fdp->serial) != 0);
	printf("%s:%d!\n", __func__, __LINE__);

	n = sendto(fd, (char[]){0x00}, 1, 0, (struct sockaddr *)&cliaddr, sizeof cliaddr);
	printf("%s:%d!\n", __func__, __LINE__);

	closesocket(fd);
	printf("%s:%d!\n", __func__, __LINE__);

	if (n == SOCKET_ERROR)
	{
		fprintf(stderr, "Failed to initiate handshake. Error Code : %d\n", WSAGetLastError());
		return NULL;
	}

	printf("%s:%d!\n", __func__, __LINE__);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof servaddr);
	// FIXME: use designated initializers, use getpeername, use C99 (gustedt)
	// TODO: couldn't I use cliaddr directly?!
	servaddr.sin_family = AF_INET;

	printf("%s:%d!\n", __func__, __LINE__);
	servaddr.sin_addr.s_addr = cliaddr.sin_addr.s_addr;
	
	union { u32 addr; u8 sub[4];} ip = {servaddr.sin_addr.s_addr};
	printf("ip: %x\n", (unsigned)cliaddr.sin_addr.s_addr);
	snprintf(fdp->ip, sizeof fdp->ip, "%u.%u.%u.%u", 
	ip.sub[0], ip.sub[1], ip.sub[2], ip.sub[3]);
	servaddr.sin_port = htons(fdp->tcp_port);
		printf("@@@@ %s:%u @@@@\n",fdp->ip, fdp->tcp_port);
	printf("%s:%d!\n", __func__, __LINE__);
	connect(fd, (struct sockaddr *)&servaddr, sizeof servaddr);
	if (n == SOCKET_ERROR)
	{
		closesocket(fd);
		fprintf(stderr, "Failed to initiate TCP connection. Error Code : %d\n", WSAGetLastError());
		return NULL;
	}
	n = snprintf(buffer, sizeof buffer, 
			"GET /target?sn=%s VMTP1.0\nProtocol: %s",
			fdp->serial, fdp->protocol);
	printf("buffer: %s\n", buffer);
	printf("%s:%d!\n", __func__, __LINE__);

	n = sendto(fd, buffer, n, 0, (struct sockaddr *)&servaddr, sizeof servaddr);
	if (n == SOCKET_ERROR)
	{
		closesocket(fd);
		fprintf(stderr, "Failed to handshake over TCP. Error Code : %d\n", WSAGetLastError());
		return NULL;
	}
	printf("%s:%d!\n", __func__, __LINE__);
	n=recvfrom(fd, buffer, sizeof buffer, 0, NULL, NULL);
	if (n == SOCKET_ERROR)
	{
		closesocket(fd);
		fprintf(stderr, "Failed to recieve TCP-connection confirmation. Error Code : %d\n", WSAGetLastError());
		return NULL;
	}
	printf("%s:%d!\n", __func__, __LINE__);
	buffer[n] = '\0';
	printf("buffer=%s\n", buffer);
	printf("%s:%d!\n", __func__, __LINE__);
	fdp->sock = (LPVOID)fd;
	return fdp;
}


/**
 * \param [in] device handle returned by tcp_open()
 * \brief releases the socket file descriptor opened by tcp_open()
 */
void tcp_close(void *handle)
{
	shutdown(*(int*)handle, SD_BOTH);
	closesocket(*(int*)handle);
	WSACleanup();
}
/**
 * \param [in] device handle returned by tcp_open()
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \bug it's useless. Could use \p wprintf and \p strerror
 */
const wchar_t *tcp_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}

int tcp_write(void* sock, const u8* buf, size_t count) {
	return send(*(SOCKET*)sock, (char*)buf, count, 0); 
}
int tcp_read(void* sock, u8* buf, size_t count, int milliseconds) {
	//FIXME: Timeout!
	(void) milliseconds;
	return recv(*(SOCKET*)sock, (char*)buf, count, 0);
}

