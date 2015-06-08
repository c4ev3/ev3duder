/**
 * @file io.c - Input/Output wrappers for Bluetooth
 * @authot Ahmad Fatoum
 * @license (c) Ahmad Fatoum. Code available under terms of the GNU General Public License
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
#define BT "/dev/cu.EV3-SerialPort"
// ^ TODO: add ability to find differently named EV3's
static void handle_sigint(int signo);
void *bt_open()
{
	//  signal(SIGINT, handle_sigint); 
	int *fd = malloc(sizeof(int));
	*fd = open(BT, O_RDWR);
	return *fd != -1 ? fd : NULL;
}

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
int bt_read(void* fd_, u8* buf, size_t count, int milliseconds)
{ // goto <3
	(void)count;
	(void)milliseconds;
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
	if (setjmp(env) == 0)
	{
again:
		for (ssize_t ret=recvd; recvd < packet_len; recvd += ret)
		{
			if (milliseconds != -1) {
				setitimer(ITIMER_REAL, &timer, NULL);
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
			if (packet_len > 2*count) //TODO: remove 2*
				return -1;
			goto again;
		}
	}
	return recvd;

}
const wchar_t *bt_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}

