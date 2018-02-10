/**
 * @file bt-unix.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Unix bluetooth I/O wrappers
 */
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

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
enum
{
	READ = 0, WRITE = 1
};

void *bt_open(const char *file, const char *file2)
{
	int *fd = malloc(2 * sizeof(int));
	if (file2)
	{
		fd[READ] = open(file ?: BT, O_RDONLY);
		fd[WRITE] = open(file ?: BT, O_WRONLY);
	}
	else
		fd[0] = fd[1] = open(file ?: BT, O_RDWR);

	if (fd[WRITE] == -1 || fd[READ] == -1) return NULL;
	return fd;
}

/**
 * \param [in] handle handle returned by bt_open()
 * \param [in] buf byte string to write, the first byte is omitted
 * \param [in] count number of characters to be written (including leading ignored byte)
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the first byte is omitted for compatiblity with the leading report byte demanded by \p hid_write. Wrapping HIDAPI could fix this.
 */
int bt_write(void *fd_, const u8 *buf, size_t count)
{
	int fd = ((int *) fd_)[WRITE];
	buf++;
	count--; // omit HID report number
	size_t sent;
	for (ssize_t ret = sent = 0; sent < count; sent += ret)
	{
		ret = write(fd, buf, count - sent);
		if (ret == -1)
		{
			// set some error msg
			break;
		}
	}
	return sent;
}

/**
 * \param [in] device handle returned by bt_open()
 * \param [in] buf buffer to write to 
 * \param [in] count number of characters to be read
 * \param [in] milliseconds number of milliseconds to wait at maximum. -1 is indefinitely
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug timeout only applies to first byte read; once something is read, the whole packet is read in a blocking way
 */
int bt_read(void *fd_, u8 *buf, size_t count, int milliseconds)
{
	int fd = ((int *) fd_)[READ];
	size_t recvd = 0;
	size_t packet_len = 2;
	ssize_t ret;

	if (milliseconds >= 0)
	{
		struct timeval timeout;
		timeout.tv_sec = milliseconds / 1000;
		timeout.tv_usec = (milliseconds % 1000) * 1000;
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		select(fd + 1, &fdset, NULL, NULL, &timeout);
		if (!FD_ISSET(fd, &fdset)) return 0;
	}

	do
	{
		ret = read(fd, buf + recvd, packet_len - recvd);
		if (ret <= 0)
		{
			perror("read failed");
			return -1;
		}
	} while ((recvd += ret) != 2);

	packet_len += buf[0] | (buf[1] << 8);

	if (packet_len > 2 * count) //FIXME: remove 2*
		return -1;

	for (ssize_t ret = recvd; recvd < packet_len; recvd += ret)
	{
		ret = read(fd, buf + recvd, packet_len - recvd);
		if (ret == 0) break; // EOF; turning off bluetooth during transfer
		//FIXME: would returning -1 for EOF make more sense?
		if (ret == -1)
		{
			perror("read failed");
			return -1;
		}
	}

	return recvd;
}

/**
 * \param [in] device handle returned by bt_open()
 * \brief Closes the file descriptor opened by bt_open()
 */
void bt_close(void *handle)
{
	close(((int *) handle)[WRITE]);
	close(((int *) handle)[READ]);
	free(handle);
}

/**
 * \param [in] device handle returned by bt_open()
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 */
const wchar_t *bt_error(void *fd_)
{
	(void) fd_;
	const char *errstr = strerror(errno);
	size_t wlen = strlen(errstr);
	wchar_t *werrstr = malloc(sizeof(wchar_t[wlen]));
	return (mbstowcs(werrstr, errstr, wlen) != (size_t) -1) ?
		   werrstr :
		   L"Error in printing error";
}

