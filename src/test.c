#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>
#include "ev3_io.h"

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
#include "funcs.h"

#ifdef __linux__
#include <sys/utsname.h>
#endif

static const u8 tone[] = "\x0\x0F\x00\0\0\x80\x00\x00\x94\x01\x81\x02\x82\xE8\x03\x82\xE8\x03";

#define MAX_STR 256

int test()
{
    int res;
    //TODO: needs more detailed information for bluetooth/wlan
    res = ev3_write(handle, tone, sizeof tone - 1);
    if (res < 0)
    {
        hiderr = ev3_error(handle);
        return ERR_HID;
    }
    errmsg = "\nAttempting beep..";
	if ((void*)ev3_write != (void*)hid_write)
    	return ERR_UNK;

#ifdef __linux__
    struct utsname utsname;
    uname(&utsname);
    int isOld = strcmp(utsname.release, "2.6.24") < 0;
    printf("SUBSYSTEM==\"%s\", ATTRS{idVendor}==\"0694\", ATTRS{idProduct}==\"0005\a, MODE=\"0666\"\n", isOld ? "usb_device" : "usb");
#endif
    wchar_t wstr[MAX_STR];
    res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
    printf("Manufacturer String: %ls\n", wstr);
    res = hid_get_product_string(handle, wstr, MAX_STR);
    printf("Product String: %ls\n", wstr);
    res = hid_get_serial_number_string(handle, wstr, MAX_STR);
    printf("Serial Number String: %ls\n", wstr);

    return ERR_UNK;
}

