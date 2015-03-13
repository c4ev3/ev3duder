#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
extern hid_device *handle;

static const u8 tone[] = "\x0\x0F\x00\0\0\x80\x00\x00\x94\x01\x81\x02\x82\xE8\x03\x82\xE8\x03";

#define MAX_STR 256

struct error ev3_test()
{
	wchar_t wstr[MAX_STR];
	int res;

	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	printf("Manufacturer String: %ls\n", wstr);
	res = hid_get_product_string(handle, wstr, MAX_STR);
	printf("Product String: %ls\n", wstr);
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	printf("Serial Number String: %ls\n", wstr);

	res = hid_write(handle, tone, sizeof tone - 1);
	if (res < 0)
	{
		return (struct error) {.category = ERR_HID, .msg = NULL, .reply = hid_error(handle)};
	}
		return (struct error) {.category = ERR_UNK, .msg = "\nAttempting beep..", .reply = NULL};
}
