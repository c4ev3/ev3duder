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

struct error rm(const char *path)
{
    int res;
    size_t path_sz = strlen(path) + 1;
    DELETE_FILE *rm_cmd = packet_alloc(DELETE_FILE, path_sz);
    memcpy(rm_cmd->path, path, path_sz);

    res = hid_write(handle, (u8 *)rm_cmd, rm_cmd->packetLen + PREFIX_SIZE);
    if (res < 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to write DELETE_FILE.", .reply = hid_error(handle)
    };
    fputs("Checking reply: \n", stderr);
    CREATE_DIR_REPLY rmrep;

    res = hid_read_timeout(handle, (u8 *)&rmrep, sizeof rmrep, TIMEOUT);
    if (res <= 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to read DELETE_FILE_REPLY", .reply = hid_error(handle)
    };

	if (rmrep.type == VM_ERROR)
	{
		const char * msg;
		if (rmrep.ret < ARRAY_SIZE(bdrep_str))
			msg = bdrep_str[rmrep.ret];
		else
			msg = "ERROR_OUT_OF_BOUNDS";
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(&rmrep, rmrep.packetLen);

		return (struct error)
		{.category = ERR_VM, .msg = "`DELETE_FILE` was denied.", .reply = msg};
	}else
    return (struct error)
    {
        .category = ERR_UNK, .msg = "`DELETE_FILE` was successful.", .reply = NULL
    };

}

