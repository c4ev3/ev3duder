// montag 1:30 - 6:30

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
/*
enum {ERR_UNK, HID_ERROR, ERR_EV3_NOT_FOUND, ERR_SD_NOT_FOUND, ERR_INVALID_PATH, ERR_TIMEOUT, ERR_END};
static const char * const ERR_STR[] = {
	[ERR_UNK] = "Unknown Error occured",
	[HID_ERROR] = "HID ERROR OCCURED.",
	[ERR_EV3_NOT_FOUND] = "EV3 not found. Is it properly plugged into the USB port?",
	[ERR_LOCAL_NOT_FOUND] = "Local path couldn't be resolved. is the file even there?"
	[ERR_REMOTE_FS_FULL] = "Remote file system is full."
	[ERR_SD_NOT_FOUND] = "Invalid destination. Is the SD_Card inserted?",
	[ERR_TIMEOUT] = "USB reply read has timed out. "
	[ERR_INVALID_PATH] = "invalid remote path. Linux doesn't like your project name.",
};*/


#define VendorID 0x694 /* LEGO GROUP */
#define ProductID 0x005 /* EV3 */
#define SerialID NULL

extern struct error ev3_up(FILE*, const char*);
extern struct error ev3_dl(const char*, FILE*);
extern struct error ev3_test(void);
extern struct error ev3_exec(const char*);
extern struct error ev3_ls(const char*);

#define arg(name, count_min, count_max) ((strcmp((name), argv[1]) == 0)   \
					&& ((count_min) <= (argc - 2))    \
					&& ((argc - 2)  <= (count_max)))

void params_print()
{
	puts(	"USAGE: ev3dude [ up loc rem | dl rem loc | exec rem | kill rem |"	"\n"
	        "                 cp rem rem | mv rem rem | rm rem | ls [rem] | tree [rem] |"	"\n"
	        "                 shell | test ]"					"\n"
	        "\n"
	        "       rem = remote (EV3) system path, loc = (local file system) path"	"\n");
}

hid_device *handle;

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		params_print();
		return ERR_ARG;
	}

	handle = hid_open(VendorID, ProductID, SerialID);
	if (!handle)
	{
		puts("EV3 not found. Is it properly plugged into the USB port?");
		return ERR_HID;
	}

	struct error ret;

	if (arg("test", 0, 0))
	{
		ret = ev3_test();
	}
	else if (arg("up", 2, 2))
	{
		FILE *fp = fopen(argv[2], "rb");
		if (!fp)
		{
			printf("File <%s> doesn't exist.\n", argv[1]);
			return ERR_IO;
		}

		ret = ev3_up(fp, argv[3]);
	}
	else if (arg("exec", 1, 1))
	{
		ret = ev3_exec(argv[2]);
	}else
	{
		params_print();
		return ERR_ARG;
	}

	if (ret.category == ERR_HID)
		fprintf(stdout, "%ls\n", (wchar_t*)ret.reply);
	else
		fprintf(stdout, "%s (%s)\n", ret.msg, (char*)ret.reply ?: "null");
	// maybe \n to stderr?
	return ret.category;
}
