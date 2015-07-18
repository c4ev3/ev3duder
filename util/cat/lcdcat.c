#include "normal_font.xbm"

#include <fcntl.h>
#include <stdio.h>
int main(int argc, char *argv[])
{
	int fd = open("/dev/fb0", O_WRONLY);
	unsigned zero = 0;
	unsigned zeroes = 0;
	scanf("%d", &zeroes);
	while (zeroes-->0)
		write(fd, &zero, sizeof(unsigned));
	close(fd);
	return 0;	
}

