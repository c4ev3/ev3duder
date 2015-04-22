#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
extern hid_device *handle;
enum {
	SUCCESS = 0,
	UNKNOWN_HANDLE,
	HANDLE_NOT_READY,
	CORRUPT_FILE,
	NO_HANDLES_AVAILABLE,
	NO_PERMISSION,
	ILLEGAL_PATH,
	FILE_EXITS,
	END_OF_FILE,
	SIZE_ERROR,
	UNKNOWN_ERROR,
	ILLEGAL_FILENAME,
	ILLEGAL_CONNECTION
};
static const char * const bdrep_str[] =
{
	[SUCCESS] 		= "SUCCESS",
	[UNKNOWN_HANDLE] 	= "UNKNOWN_HANDLE",
	[HANDLE_NOT_READY] 	= "HANDLE_NOT_READY",
	[CORRUPT_FILE] 		= "CORRUPT_FILE",
	[NO_HANDLES_AVAILABLE] 	= "NO_HANDLES_AVAILABLE\tPath doesn't resolve to a valid file",
	[NO_PERMISSION] 	= "NO_PERMISSION",
	[ILLEGAL_PATH] 		= "ILLEGAL_PATH",
	[FILE_EXITS] 		= "FILE_EXITS",
	[END_OF_FILE] 		= "END_OF_FILE",
	[SIZE_ERROR] 		= "SIZE_ERROR\tCan't write here. Is SD Card properly inserted?",
	[UNKNOWN_ERROR] 	= "UNKNOWN_ERROR\tNo such file or directory",
	[ILLEGAL_FILENAME] 	= "ILLEGAL_FILENAME",
	[ILLEGAL_CONNECTION] 	= "ILLEGAL_CONNECTION",
};

struct error ls(const char *path)
{
    int res;
    size_t path_sz = strlen(path) + 1;
    LIST_FILES *list = packet_alloc(LIST_FILES, path_sz);
    list->maxBytes = 0xffff;
    memcpy(list->path, path, path_sz);

//FIXME: inquire whether start succeeded. check communicaton developer manual  pdf (debug mode maybe?)
    print_bytes(list, list->packetLen + PREFIX_SIZE);
    res = hid_write(handle, (u8 *)list, list->packetLen + PREFIX_SIZE);
    if (res < 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to write BEGIN_DOWNLOAD.", .reply = hid_error(handle)
    };
    fputs("Checking reply: \n", stderr);
    size_t listrep_sz = sizeof(LIST_FILES_REPLY) + list->maxBytes;
    LIST_FILES_REPLY *listrep = malloc(listrep_sz);

    res = hid_read_timeout(handle, (u8 *)listrep, listrep_sz, TIMEOUT);
    if (res <= 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to read LIST_FILES_REPLY", .reply = hid_error(handle)
    };

	if (listrep->type == VM_ERROR)
	{
		const char * msg;
		if (listrep->ret < ARRAY_SIZE(bdrep_str))
			msg = bdrep_str[listrep->ret];
		else
			msg = "ERROR_OUT_OF_BOUNDS";
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(listrep, listrep->packetLen);

		return (struct error)
		{.category = ERR_VM, .msg = "`LIST_FILES` was denied.", .reply = msg};
	}
    puts(listrep->list);
    //README: it's assumed that the folder structure fits into ONE HID packet. if not additional code for handling CONTINUTE_LIST_FILES is required
    return (struct error)
    {
        .category = ERR_UNK, .msg = "`LIST_FILES` was successful.", .reply = NULL
    };

}

