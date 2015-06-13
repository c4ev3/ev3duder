#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <windows.h>
#include "src/btserial.h"
static const unsigned char tone[] = "\x0\x0F\x00\0\0\x80\x00\x00\x94\x01\x81\x02\x82\xE8\x03\x82\xE8\x03";

#define MAX_STR 256

int main(int argc, char *argv[])
{
	HANDLE handle = bt_open(L"COM3");
	int written = bt_write(handle, tone, sizeof tone - 1);
	CloseHandle(handle);
    printf("Wrote %d bytes of %u", written, sizeof tone-1);
	return 0;
}

