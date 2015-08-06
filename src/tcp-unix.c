/**
 * @file tcp-unix.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief BSD socket I/O wrappers
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <signal.h>

#include "defs.h"
#include "tcp.h"
#include "btserial.h"
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
	struct tcp_handle *fdp = malloc(sizeof(fdp));
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Failed to create socket");
		return NULL;
	}
	struct sockaddr_in servaddr, cliaddr;
	memset(&servaddr, 0, sizeof servaddr);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port = htons(UDP_PORT);
	if (bind(fd, (struct sockaddr *)&servaddr, sizeof servaddr) == -1)
	{
		close(fd);
		perror("Failed to bind");
		return NULL;
	}
    socklen_t len = sizeof cliaddr;
	char buffer[128];
	ssize_t n;
	do {
	n = recvfrom(fd, buffer,1000,0,(struct sockaddr *)&cliaddr,&len);
	if (n < 0)
	{
		close(fd);
		perror("Failed to recieve broadcast"); // is that even a thing for nonblocking socks?
		return NULL;
	}
    buffer[n] = '\0';
	sscanf(buffer, 
	"Serial-Number: %s\r\n"
	"Port: %u\r\n"
	"Name: %s\r\n"
	"Protocol: %s\r\n",
	fdp->serial, &fdp->tcp_port, fdp->name, fdp->protocol);

	}while(serial && strcmp(serial, fdp->serial) == 0);

	n = sendto(fd, (char[]){0x00}, 1, 0, (struct sockaddr *)&cliaddr, sizeof cliaddr);
	close(fd);
	if (n < 0)
	{
		perror("Failed to initiate handshake");
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
	if (n < 0)
	{
		close(fd);
		perror("Failed to initiate TCP connection");
		return NULL;
	}
	n = snprintf(buffer, sizeof buffer, "GET /target?sn=%s VMTP1.0\nProtocol: %s",
			fdp->serial, fdp->protocol);

	n = sendto(fd, buffer, n, 0, (struct sockaddr *)&servaddr, sizeof servaddr);
	if (n < 0)
	{
		close(fd);
		perror("Failed to handshake over TCP");
		return NULL;
	}
	n=recvfrom(fd, buffer, sizeof buffer, 0, NULL, NULL);
	if (n < 0)
	{
		close(fd);
		perror("Failed to recieve TCP-connection confirmation"); 
		return NULL;
	}
	buffer[n] = '\0';
	//printf("buffer=%s\n", buffer);
	fdp->fd[0] = fdp->fd[1] = fd;
	return fdp;
}



/**
 * \param [in] handle handle returned by bt_open()
 * \param [in] buf byte string to write, the first byte is omitted
 * \param [in] count number of characters to be written (including leading ignored byte)
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the first byte is omitted for compatiblity with the leading report byte demanded by \p hid_write. Wrapping HIDAPI could fix this.
 */ 

/**
 * \param [in] device handle returned by bt_open()
 * \param [in] buf buffer to write to 
 * \param [in] count number of characters to be read
 * \param [in] milliseconds number of milliseconds to wait at maximum. -1 is indefinitely
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the milliseconds part needs to be tested more throughly
 */ 

/**
 * \param [in] device handle returned by bt_open()
 * \brief Closes the file descriptor opened by bt_open()
 */
void tcp_close(void *handle)
{
	shutdown(*(int*)handle, SHUT_RDWR);
	close(*(int*)handle);
	free(handle);
}
/**
 * \param [in] device handle returned by bt_open()
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \bug it's useless. Could use \p wprintf and \p strerror
 */
const wchar_t *tcp_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}

int (*tcp_write)(void* device, const u8* buf, size_t count) = bt_write;
int (*tcp_read)(void* device, u8* buf, size_t count, int milliseconds) = bt_read;

