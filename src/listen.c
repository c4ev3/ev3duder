#include <stdio.h>
#include <hidapi.h>
#include <string.h>
#include "defs.h"
#include "ev3_io.h"

int listen()
{
	u8 buffer[1280] = {0};
	int read;
	while ( (read = hid_read(handle,buffer, sizeof buffer)) > 0)
	{
		printf("%d bytes read.\n", read);
		   //print_bytes(buffer, read);	
		   printf("[%s]\n", buffer);
		   memset(buffer, 0, sizeof buffer);
	}

	return 0;
}

