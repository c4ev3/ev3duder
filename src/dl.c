/**
 * @file dl.c
 * @brief untested
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "ev3_io.h"

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

//! Transmission is done in units smaller or equal to CHUNK_SIZE
#define CHUNK_SIZE 1000 // EV3's HID driver doesn't do packets > 1024B
/**
 * \param path path on the ev3
 * \param fp FILE* to write data to
 * \retval error according to enum #ERR
 * \bug doesn't work, needs more debugging (over WiFi maybe)
 */
int dl(const char *path, FILE *fp)
{
	int res;
	size_t path_sz = strlen(path) + 1;

	BEGIN_UPLOAD *bu = packet_alloc(BEGIN_UPLOAD, path_sz);
	memcpy(bu->fileName, path, path_sz);
	bu->maxBytes = CHUNK_SIZE;

	print_bytes(bu, bu->packetLen + PREFIX_SIZE);
	res = ev3_write(handle, (u8 *)bu, bu->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write BEGIN_UPLOAD.";
		return ERR_COMM;
	}
	fputs("Checking reply: \n", stderr);
	size_t file_chunksz = sizeof(BEGIN_UPLOAD_REPLY) + bu->maxBytes;
	void *file_chunk = malloc(file_chunksz);

	BEGIN_UPLOAD_REPLY *burep = file_chunk;

	res = ev3_read_timeout(handle, (u8 *)burep, file_chunksz, TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read BEGIN_UPLOAD";
		return ERR_COMM;
	}

	if (burep->type == VM_ERROR)
	{
		errno = burep->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(burep, burep->packetLen);


		errmsg = "`BEGIN_UPLOAD` was denied.";
		return ERR_VM;
	}
	unsigned read_so_far = burep->packetLen + 2 - offsetof(BEGIN_UPLOAD_REPLY, bytes);
	fwrite(burep->bytes, read_so_far, 1, fp);

	unsigned total = burep->fileSize;

	CONTINUE_UPLOAD cu = CONTINUE_UPLOAD_INIT;
	cu.fileHandle =burep->fileHandle;
	cu.maxBytes = CHUNK_SIZE + sizeof(BEGIN_UPLOAD_REPLY) - sizeof(CONTINUE_UPLOAD_REPLY);
	fprintf(stderr, "read %u from total %u bytes.\n", read_so_far, total);
	CONTINUE_UPLOAD_REPLY *curep = file_chunk;
	while(read_so_far < total)
	{
		res = ev3_write(handle, (u8*)&cu, sizeof cu);
		if (res < 0)
		{
			errmsg = "Unable to write CONTINUE_UPLOAD";
			return ERR_COMM;
		}

		res = ev3_read_timeout(handle, (u8 *)curep, file_chunksz, TIMEOUT);
		if (res <= 0)
		{
			errmsg = "Unable to read CONTINUE_UPLOAD_REPLY";
			return ERR_COMM;
		}
		if (curep->type == VM_ERROR)
		{
			errno = curep->ret;
			fputs("Operation failed.\nlast_reply=", stderr);
			print_bytes(curep, curep->packetLen);

			errmsg = "`CONTINUE_UPLOAD` was denied.";
			return ERR_VM;
		}	
		size_t read_this_time = curep->packetLen + 2 - offsetof(CONTINUE_UPLOAD_REPLY, bytes);
		fwrite(curep->bytes, read_this_time, 1, fp);
		cu.fileHandle = curep->fileHandle;
		read_so_far += read_this_time;
		fprintf(stderr, "read %u from total %u bytes.\n", read_so_far, total);
	}

	errmsg = "`BEGIN_UPLOAD` was successful.";
	return ERR_UNK;

}


