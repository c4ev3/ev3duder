#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
extern hid_device *handle;


#define VendorID 0x694 /* LEGO GROUP */
#define ProductID 0x005 /* EV3 */
#define SerialID NULL
#define TIMEOUT 2000 /* in milli seconds */


#define MAX_STR 256

#define CHUNK_SIZE 1000 //FIXME: for some reason HIDAPI refuses sending more bytes?!
//(((u16)~0 - sizeof (CONTINUE_DOWNLOAD) - PREFIX_SIZE)/1)

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
	[UNKNOWN_ERROR] 	= "UNKNOWN_ERROR",
	[ILLEGAL_FILENAME] 	= "ILLEGAL_FILENAME",
	[ILLEGAL_CONNECTION] 	= "ILLEGAL_CONNECTION",
};

struct error ev3_up(FILE *fp, const char *dst)
{
	int res;

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp); // file size limit is min(LONG_MAX, 4gb)
	size_t extra_chunks   = fsize / CHUNK_SIZE;
	size_t final_chunk_sz = fsize % CHUNK_SIZE;

	fprintf(stderr, "Attempting file upload (%ldb total; %u chunks): \n", fsize, extra_chunks + 1);
	fseek(fp, 0, SEEK_SET);

	CONTINUE_DOWNLOAD **cd = malloc((1 + extra_chunks) * sizeof(*cd));
	{
		size_t ret;
		size_t i = 0;
		for (; i < extra_chunks; ++i)
		{
			cd[i] = packet_alloc(CONTINUE_DOWNLOAD, CHUNK_SIZE);
			ret = fread(cd[i]->fileChunk, 1, CHUNK_SIZE, fp);
		}

		cd[i] = packet_alloc(CONTINUE_DOWNLOAD, final_chunk_sz);

		ret = fread(cd[i]->fileChunk, 1, final_chunk_sz, fp);
		(void) ret;
		fclose(fp);
	}

	//TODO: read in chunks, whatif long isnt big enough
	BEGIN_DOWNLOAD *bd = packet_alloc(BEGIN_DOWNLOAD, strlen(dst) + 1);
	bd->fileSize = fsize;
	strcpy(bd->fileName, dst);

	res = hid_write(handle, (u8 *)bd, bd->packetLen + PREFIX_SIZE);
	if (res < 0) return (struct error)
	{.category = ERR_HID, .msg = "Unable to write BEGIN_DOWNLOAD.", .reply = hid_error(handle)};

	fputs("Checking reply: \n", stderr);

	BEGIN_DOWNLOAD_REPLY bdrep;

	res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
	if (res <= 0) return (struct error)
	{.category = ERR_HID, .msg = "Unable to read BEGIN_DOWNLOAD_REPLY", .reply = hid_error(handle)};

	if (bdrep.type == VM_ERROR)
	{
		const char *msg;
		if (bdrep.ret < ARRAY_SIZE(bdrep_str))
			msg = bdrep_str[bdrep.ret];
		else
			msg = "ERROR_OUT_OF_BOUNDS";

		return (struct error)
		{.category = ERR_VM, .msg = "BEGIN_DOWNLOAD was denied.", .reply = msg};
	}


#ifdef DEBUG
	fputs("=BEGIN_DOWNLOAD_REPLY", stderr);
	print_bytes(&bdrep, sizeof(bdrep));
	putc('\n', stderr);
	fprintf(stderr, "current handle is %u\n", bdrep.fileHandle);
	fputs("=cut", stderr);
#endif

	for (size_t i = 0; i <= extra_chunks; ++i)
	{
		cd[i]->fileHandle = bdrep.fileHandle;
		res = hid_write(handle, (u8 *)cd[i], cd[i]->packetLen + PREFIX_SIZE);
#if DEBUG > 7
		fprintf(stderr, "=CONTINUE_DOWNLOAD");;
		print_bytes(cd[i], cd[i]->packetLen + PREFIX_SIZE);
		fputs("=cut", stderr);
#endif
		if (res < 0) return (struct error)
		{.category = ERR_HID, .msg = "Unable to write CONTINUE_DOWNLOAD.", .reply = hid_error(handle)};

		res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
		if (res <= 0) return (struct error)
		{.category = ERR_HID, .msg = "Unable to read CONTINUE_DOWNLOAD_REPLY", .reply = hid_error(handle)};


		fprintf(stderr,"=CONTINUE_DOWNLOAD_REPLY(chunk=%u, data=%ub)\n", i,
		       cd[i]->packetLen - (sizeof(CONTINUE_DOWNLOAD) - PREFIX_SIZE));
		print_bytes(&bdrep, PREFIX_SIZE + bdrep.packetLen);
		fputs("=cut", stderr);
	}


	//printf("lol?");
	/*res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
	if (res == 0)
		die("Request timed out.");
	if (res < 0)
		die("Unable to read.");
	printf("lol!");*/

	if (bdrep.type == VM_ERROR)
	{
		const char * msg;
		if (bdrep.ret < ARRAY_SIZE(bdrep_str))
			msg = bdrep_str[bdrep.ret];
		else
			msg = "ERROR_OUT_OF_BOUNDS";

		fputs("Transfer failed.\nlast_reply=", stderr);

		print_bytes(&bdrep, bdrep.packetLen);

		return (struct error)
		{.category = ERR_VM, .msg = "CONTINUE_DOWNLOAD was denied.", .reply = msg};
	}

	fprintf(stderr, "Transfer has been successful! (ret=%d)\n", bdrep.type);

	return (struct error)
	{.category = ERR_UNK, .msg = "`upload` was successful.", .reply = NULL};
}
