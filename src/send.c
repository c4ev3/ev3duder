#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "ev3_io.h"

int sendout()
{
	char buffer[1023];
	buffer[0] = 0;
	int written;
	while (fgets(buffer + 1, sizeof buffer - 1, stdin))
	{
		size_t len = strlen(buffer + 1) + 1;
		written = ev3_write(handle, (u8 *) buffer, len);
		printf("%d bytes read.\n", written);
	}

	return 0;
}

