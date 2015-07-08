/**
 * @file dl.c
 * @brief untested
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	if (!fp)
		fp = fopen(strrchr(path, '/') + 1, "w");

    BEGIN_UPLOAD *bu = packet_alloc(BEGIN_UPLOAD, path_sz);
    memcpy(bu->fileName, path, path_sz);
	bu->maxBytes = CHUNK_SIZE;

    print_bytes(bu, bu->packetLen + PREFIX_SIZE);
    res = ev3_write(handle, (u8 *)bu, bu->packetLen + PREFIX_SIZE);
    if (res < 0)
    {
        errmsg = "Unable to write BEGIN_UPLOAD.";
        hiderr = ev3_error(handle);
        return ERR_HID;
    }
    fputs("Checking reply: \n", stderr);
    size_t file_chunksz = sizeof(BEGIN_UPLOAD_REPLY) + bu->maxBytes;
    void *file_chunk = malloc(file_chunksz);
	BEGIN_UPLOAD_REPLY *burep = file_chunk;

    res = ev3_read_timeout(handle, (u8 *)burep, file_chunksz, TIMEOUT);
    if (res <= 0)
    {
        errmsg = "Unable to read BEGIN_UPLOAD";
        hiderr = ev3_error(handle);
        return ERR_HID;
    }

    if (burep->type == VM_ERROR)
    {
        if (burep->ret < ARRAY_SIZE(ev3_error_msgs))
            hiderr = ev3_error_msgs[burep->ret];
        else
            hiderr = L"ERROR_OUT_OF_BOUNDS";
        fputs("Operation failed.\nlast_reply=", stderr);
        print_bytes(burep, burep->packetLen);


        errmsg = "`BEGIN_UPLOAD` was denied.";
        return ERR_VM;
    }
    fwrite(burep->bytes, CHUNK_SIZE, 1, fp);

	size_t read_so_far = burep->packetLen + 2 - offsetof(BEGIN_UPLOAD_REPLY, bytes);
	size_t total = burep->fileSize;
	free(burep);

    CONTINUE_UPLOAD cu = CONTINUE_UPLOAD_INIT;
	cu.fileHandle =burep->fileHandle;
	cu.maxBytes = CHUNK_SIZE + sizeof(BEGIN_UPLOAD_REPLY) - sizeof(CONTINUE_UPLOAD_REPLY);
	fprintf(stderr, "read %zu from total %zu bytes.\n", read_so_far, total);
	CONTINUE_UPLOAD_REPLY *curep = file_chunk;
	int ret = curep->ret;
	while(ret != END_OF_FILE)
	{
	res = ev3_write(handle, (u8*)&cu, sizeof cu);
    if (res < 0)
    {
		errmsg = "Unable to write BEGIN_UPLOAD";
        hiderr = ev3_error(handle);
        return ERR_HID;
    }

	    res = ev3_read_timeout(handle, (u8 *)curep, file_chunksz, TIMEOUT);
    	if (res <= 0)
    	{
			errmsg = "Unable to read BEGIN_UPLOAD_REPLY";
			hiderr = ev3_error(handle);
			return ERR_HID;
    	}
		if (curep->type == VM_ERROR)
		{
			if (curep->ret < ARRAY_SIZE(ev3_error_msgs))
				hiderr = ev3_error_msgs[curep->ret];
			else
				hiderr = L"ERROR_OUT_OF_BOUNDS";
			fputs("Operation failed.\nlast_reply=", stderr);
			print_bytes(curep, curep->packetLen);


			errmsg = "`BEGIN_UPLOAD` was denied.";
			return ERR_VM;
		}	
		fwrite(burep->bytes, cu.maxBytes, 1, fp);
		cu.fileHandle = curep->fileHandle;
	size_t read_so_far = curep->packetLen + 2 - offsetof(CONTINUE_UPLOAD_REPLY, bytes);
	fflush(stdout);
	fprintf(stderr, "read %zu from total %zu bytes.\n", read_so_far, file_chunksz);
	ret = curep->ret;
	}

    errmsg = "`BEGIN_UPLOAD` was successful.";
    return ERR_UNK;

}


