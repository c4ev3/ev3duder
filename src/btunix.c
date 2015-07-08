/**
 * @file btunix.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 2.0
 * @brief Unix bluetooth I/O wrappers
 */
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>

#include "defs.h"
//! default serial port name on OS X
#define BT "/dev/cu.EV3-SerialPort"
// ^ TODO: add ability to find differently named EV3's
/**
 * \param [in] device path to SerialPort or \p NULL
 * \return &fd pointer to file descriptor for use with bt_{read,write,close,error}
 * \brief open(2)s serial port  described by device. `NULL` leads to default action
 * \bug default value should be enumerating. Not hardcoded like in \p BT
 */ 
void *bt_open(const char *file)
{
	//  signal(SIGINT, handle_sigint);
	int *fd = malloc(sizeof(int));
	*fd = open(file ?: BT, O_RDWR);
	return *fd != -1 ? fd : NULL;
}

/**
 * \param [in] handle handle returned by bt_open()
 * \param [in] buf byte string to write, the first byte is omitted
 * \param [in] count number of characters to be written (including leading ignored byte)
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the first byte is omitted for compatiblity with the leading report byte demanded by \p hid_write. Wrapping HIDAPI could fix this.
 */ 
int bt_write(void* fd_, const u8* buf, size_t count)
{
	size_t sent;
	int fd = *(int*)fd_;
	buf++;count--; // omit HID report number
	for (ssize_t ret = sent = 0; sent < count; sent += ret)
	{
		ret = write(fd, buf, count-sent);
		if (ret == -1)
		{
			// set some error msg
			break; 
		}
	}
	return sent;
}
static jmp_buf env;
static void handle_alarm(int sig)
{
	(void)sig; longjmp(env, 1);
}

/**
 * \param [in] device handle returned by bt_open()
 * \param [in] buf buffer to write to 
 * \param [in] count number of characters to be read
 * \param [in] milliseconds number of milliseconds to wait at maximum. -1 is indefinitely
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the milliseconds part needs to be tested more throughly
 */ 
int bt_read(void* fd_, u8* buf, size_t count, int milliseconds)
{
	int fd = *(int*)fd_;
	size_t recvd =0;
	size_t packet_len = 2;
	signal(SIGALRM, handle_alarm);
	struct itimerval timer;
	milliseconds = 1;
	if (milliseconds != -1)
	{
		timer.it_value.tv_sec = milliseconds / 1000;
		timer.it_value.tv_usec = milliseconds % 1000 * 1000;
	}
	while(setjmp(env) == 0)
	{
		for (ssize_t ret=recvd; recvd < packet_len; recvd += ret)
		{
			if (milliseconds != -1) {
				setitimer(ITIMER_REAL, &timer, NULL);  //TODO: maybe move this out the loop? would handle diosconnects that way
				ret = read(fd, buf+recvd, packet_len-recvd);
				alarm(0);
			}else
			ret = read(fd, buf+recvd, packet_len-recvd);
			if (ret == -1)
			{
				perror("read failed");
				return -1; 
			}
			// bug one should handle disconnects during transfer. if this happens read keeps returning zeros
		}
		if (recvd == 2)
		{
			packet_len += buf[0] | (buf[1] << 8);
			if (packet_len > 2*count) //FIXME: remove 2*
				return -1;
			continue;
		}
		break;
	}
	return recvd;

}

/**
 * \param [in] device handle returned by bt_open()
 * \brief Closes the file descriptor opened by bt_open()
 */
void bt_close(void *handle)
{
	close(*(int*)handle);
	free(handle);
}
/**
 * \param [in] device handle returned by bt_open()
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \bug it's useless. Could use \p wprintf and \p strerror
 */
const wchar_t *bt_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}

