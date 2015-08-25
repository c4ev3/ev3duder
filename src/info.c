/**
 * @file info.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief info function - beeps and prints info
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi/hidapi.h>
#include "tcp.h"
#include "ev3_io.h"

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

#ifdef __linux__
#include <sys/utsname.h>
#endif

//! Full HID packet for causing a beep
static const u8 tone[] = "\x0\x0F\x00\0\0\x80\x00\x00\x94\x01\x81\x02\x82\xE8\x03\x82\xE8\x03";

#define MAX_STR 256

/**
 * \retval err_code An error code according to `enum #ERR`
 * \brief cause a beep and print HID information if applicable
 * \bug needs more detailed information for bluetooth/wlan
 * 		should print COM port, device name
 */ 
int info(const char*arg)
{
	wchar_t wstr[MAX_STR];
	if (arg)
	{
		if((void*)ev3_close == (void*)hid_close)
		{
			if (strcmp(arg, "serial") == 0)
			{
				hid_get_serial_number_string(handle, wstr, MAX_STR);
				fputws(wstr, stdout);
				fputwc(L'\n', stdout);
				return ERR_UNK;
			}
		}else if(ev3_close == tcp_close)
		{
			if (strcmp(arg, "ip") == 0)
			{
				puts(((struct tcp_handle*)handle)->ip);
				return ERR_UNK;
			}
		}

		return ERR_ARG;
	}

	int res;
	//TODO: 
	res = ev3_write(handle, tone, sizeof tone - 1);
	if (res < 0)
	{
		return ERR_COMM;
	}
	errmsg = "\nAttempting beep..";
	if ((void*)ev3_write != (void*)hid_write)
		return ERR_UNK;

#ifdef __linux__
	struct utsname utsname;
	uname(&utsname);
	int isOld = strcmp(utsname.release, "2.6.24") < 0;
	//! Print udev rules
	printf("SUBSYSTEM==\"%s\", ATTRS{idVendor}==\"0694\", ATTRS{idProduct}==\"0005\a, MODE=\"0666\"\n", isOld ? "usb_device" : "usb");
#endif
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	printf("Manufacturer String: %ls\n", wstr);
	res = hid_get_product_string(handle, wstr, MAX_STR);
	printf("Product String: %ls\n", wstr);
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	printf("Serial Number String: %ls\n", wstr);

	return ERR_UNK;
}

