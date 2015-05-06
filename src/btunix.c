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

#include "defs.h"
#define BT "/dev/cu.EV3-SerialPort"
// ^ TODO: add ability to find differently named EV3's
static void handle_sigint(int signo);
void *bt_open()
{
  signal(SIGINT, handle_sigint); 
  int *fd = malloc(sizeof(int));
  *fd = open(BT, O_RDWR | O_NONBLOCK);
  fprintf(stderr, "bt_open reporting. fd = %d ",*fd);
  return fd;
}

int bt_write(void* fd_, const u8* buf, size_t count)
{
  size_t sent;
  int fd = *(int*)fd_;
  for (size_t ret = sent = 0; sent < count; sent += ret)
  {
    //ssize_t ret = write(fd, buf+sent, count-sent);
    ssize_t ret = write(fd, buf, count);
    if (ret < 0)
    {
      // set some error msg
      break; 
    }
  }
  return sent;
}
int bt_read(void* fd_, u8* buf, size_t count, int milliseconds)
{
  int fd = *(int*)fd_;
  if(0 && milliseconds > -1)
  {
    struct termios termios;
    tcgetattr(fd, &termios);
    termios.c_lflag &= ~ICANON;
    termios.c_cc[VTIME] = milliseconds / 100; 
    tcsetattr(fd, TCSANOW, &termios);
  }// else blocking wait

  size_t recvd;
  for (size_t ret = recvd = 0; recvd < count; recvd += ret)
  {
    ssize_t ret = read(fd, buf+recvd, count-recvd);
    if (ret < 0)
    {
      // set some error msg maybe save errno in static field
      break; 
    }
  }
  return recvd;

}
const wchar_t *bt_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}
static void handle_sigint(int signo)
{
  switch(signo)
  {
    case SIGINT:
      exit(1);
  } 
}

