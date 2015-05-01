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
	[UNKNOWN_ERROR] 	= "UNKNOWN_ERROR\tNo such directory",
	[ILLEGAL_FILENAME] 	= "ILLEGAL_FILENAME",
	[ILLEGAL_CONNECTION] 	= "ILLEGAL_CONNECTION",
};

struct error mkdir(const char *path)
{
    int res;
    size_t path_sz = strlen(path) + 1;
    CREATE_DIR *mk = packet_alloc(CREATE_DIR, path_sz);
    memcpy(mk->path, path, path_sz);

    res = hid_write(handle, (u8 *)mk, mk->packetLen + PREFIX_SIZE);
    if (res < 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to write CREATE_DIR.", .reply = hid_error(handle)
    };
    fputs("Checking reply: \n", stderr);
    CREATE_DIR_REPLY mkrep;

    res = hid_read_timeout(handle, (u8 *)&mkrep, sizeof mkrep, TIMEOUT);
    if (res <= 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to read CREATE_DIR_REPLY", .reply = hid_error(handle)
    };

	if (mkrep.type == VM_ERROR)
	{
		const char * msg;
		if (mkrep.ret < ARRAY_SIZE(bdrep_str))
			msg = bdrep_str[mkrep.ret];
		else
			msg = "ERROR_OUT_OF_BOUNDS";
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(&mkrep, mkrep.packetLen);

		return (struct error)
		{.category = ERR_VM, .msg = "`CREATE_DIR` was denied.", .reply = msg};
	}else
    return (struct error)
    {
        .category = ERR_UNK, .msg = "`CREATE_DIR` was successful.", .reply = NULL
    };

}

