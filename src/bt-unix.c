/**
 * @file bt-unix.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Unix bluetooth I/O wrappers
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>

#include "defs.h"


//! default serial port name on OS X
#define BT "/dev/cu.EV3-SerialPort"
// ^ TODO: add ability to find differently named EV3's


typedef struct {
	int read_fd;
	int write_fd;
	struct termios old_read_mode;
	struct termios old_write_mode;
} bt_handle_t;

int set_termios(struct termios *old, int fd) {
	if (tcgetattr(fd, old) < 0) {
		perror("cannot get tty attributes");
		return -1;
	}

	struct termios new = *old;
	new.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | IGNPAR | INLCR | INPCK | ISTRIP | IXOFF | IXON | PARMRK);
	new.c_oflag &= ~(OPOST);
	new.c_cflag |= (CREAD | CS8);
	new.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	new.c_cc[VMIN]  = 1;
	new.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &new) < 0) {
		perror("cannot set tty attributes");
		return -1;
	}
	return 0;
}

/**
 * \param [in] device path to SerialPort or \p NULL
 * \return bt_handle_t with file descriptors for use with bt_{read,write,close,error}
 * \brief open(2)s serial port  described by device. `NULL` leads to default action
 * \bug default value should be enumerating. Not hardcoded like in \p BT
 */
void *bt_open(const char *file, const char *file2)
{
	bt_handle_t *cb = malloc(sizeof(bt_handle_t));
	if (file2)
	{
		cb->read_fd = open(file ?: BT, O_RDONLY | O_NOCTTY);
		cb->write_fd = open(file ?: BT, O_WRONLY | O_NOCTTY);
	}
	else
		cb->read_fd = cb->write_fd = open(file ?: BT, O_RDWR | O_NOCTTY);

	if (cb->write_fd == -1 || cb->read_fd == -1) return NULL;

	// order read -> write
	if (set_termios(&cb->old_read_mode,  cb->read_fd)  < 0) return NULL;
	if (set_termios(&cb->old_write_mode, cb->write_fd) < 0) return NULL;
	return cb;
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
	bt_handle_t *cb = (bt_handle_t *) fd_;
	buf++;
	count--; // omit HID report number
	size_t sent;
	for (ssize_t ret = sent = 0; sent < count; sent += ret)
	{
		ret = write(cb->write_fd, buf, count - sent);
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
	bt_handle_t *cb = (bt_handle_t *) fd_;
	int fd = cb->read_fd;
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
	bt_handle_t *cb = (bt_handle_t *) handle;

	// reverse order (write -> read) is important
	if (tcsetattr(cb->write_fd, TCSAFLUSH, &cb->old_write_mode) < 0)
		perror("cannot revert tty attributes");
	if (tcsetattr(cb->read_fd,  TCSAFLUSH, &cb->old_read_mode)  < 0)
		perror("cannot revert tty attributes");

	close(cb->write_fd);
	close(cb->read_fd);
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
