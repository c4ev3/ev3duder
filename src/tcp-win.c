/**
 * @file tcp-win.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Winsock2 I/O wrappers
 */

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
	(void) serial;
	return NULL;
}


/**
 * \param [in] device handle returned by tcp_open()
 * \brief releases the socket file descriptor opened by tcp_open()
 */
void tcp_close(void *handle)
{
	(void)handle;
	return;
}
/**
 * \param [in] device handle returned by tcp_open()
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \bug it's useless. Could use \p wprintf and \p strerror
 */
const wchar_t *tcp_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}

int (*tcp_write)(void* device, const u8* buf, size_t count) = NULL;
int (*tcp_read)(void* device, u8* buf, size_t count, int milliseconds) = NULL;

