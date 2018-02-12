/**
 * @file up.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief uploads a file to the ev3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ev3_io.h"

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

#define CHUNK_SIZE 1000 // The EV3 doesn't do packets > 1024B

/**
 * @brief Uploads file to the ev3. path is always relative to <em>/home/root/lms2012/prjs/sys/<em>
 * @param [in] fp Local FILE* to upload
 * @param [in] dst destination path to upload to (UTF-8 encoded)
 * @retval error according to enum #ERR
 * @bug might not handle files bigger than 2gb :-)
 */
int up(FILE *fp, const char *dst)
{
	int res;

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if ((unsigned long) fsize > (UINT32_MAX) -1)
		return ERR_FTOOBIG;

	unsigned long final_chunk_sz = (unsigned long)(fsize % CHUNK_SIZE);
	unsigned long chunks = (unsigned long)(fsize / CHUNK_SIZE);

	// final_chunk_sz == 0 ==> fsize is a multiple of CHUNK_SIZE
	// So the final_chunk_sz is supposed CHUNK_SIZE and not 0
	// If final_chunk_sz != 0 add one to chunks, so we can send the final packet in the loop
	if (final_chunk_sz == 0)
		final_chunk_sz = CHUNK_SIZE;
	else
		chunks++;

	//fprintf(stderr, "Attempting file upload (%ldb total; %u chunks): \n", fsize, chunks + 1);

	size_t dst_len = strlen(dst) + 1;
	BEGIN_DOWNLOAD *bd = packet_alloc(BEGIN_DOWNLOAD, dst_len);
	bd->fileSize = (u32)fsize;
	memcpy(bd->fileName, dst, dst_len);

	res = ev3_write(handle, (u8 *) bd, bd->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write BEGIN_DOWNLOAD.";
		return ERR_COMM;
	}

	BEGIN_DOWNLOAD_REPLY bdrep;
	res = ev3_read_timeout(handle, (u8 *) &bdrep, sizeof bdrep, TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read BEGIN_DOWNLOAD";
		return ERR_COMM;
	}

	if (bdrep.type == VM_ERROR)
	{
		errno = bdrep.ret;
		errmsg = "BEGIN_DOWNLOAD was denied.";
		return ERR_VM;
	}


	CONTINUE_DOWNLOAD *cd = packet_alloc(CONTINUE_DOWNLOAD, CHUNK_SIZE);
	for (size_t i = 0; i < chunks; i++)
	{
		cd->fileHandle = bdrep.fileHandle;
		// If i == chunks-1 (if this is the final chunk), use the proper packetLen
		if (i == chunks-1)
			cd->packetLen = sizeof(CONTINUE_DOWNLOAD) + final_chunk_sz - PREFIX_SIZE;

		// If i == chunks-1 (if this is the final chunk), use final_chunk_sz instead of CHUNK_SIZE
		fread(cd->fileChunk, 1, (i == chunks-1) ? final_chunk_sz : CHUNK_SIZE, fp);
		res = ev3_write(handle, (u8 *) cd, cd->packetLen + PREFIX_SIZE);

		if (res < 0)
		{
			errmsg = "Unable to write CONTINUE_DOWNLOAD.";
			return ERR_COMM;
		}

		res = ev3_read_timeout(handle, (u8 *) &bdrep, sizeof bdrep, TIMEOUT);
		if (res <= 0)
		{
			errmsg = "Unable to read CONTINUE_DOWNLOAD";
			return ERR_COMM;
		}
	}


	/*
	cd->packetLen = sizeof(CONTINUE_DOWNLOAD) + final_chunk_sz - PREFIX_SIZE;
	fread(cd->fileChunk, 1, final_chunk_sz, fp);
	res = ev3_write(handle, (u8 *) cd, cd->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write CONTINUE_DOWNLOAD.";
		return ERR_COMM;
	}

	res = ev3_read_timeout(handle, (u8 *) &bdrep, sizeof bdrep, TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read CONTINUE_DOWNLOAD";
		return ERR_COMM;
	}
	 */

	if (bdrep.type == VM_ERROR)
	{
		errno = bdrep.ret;

		fputs("Transfer failed.\nlast_reply=", stderr);

		print_bytes(&bdrep, bdrep.packetLen);

		errmsg = "CONTINUE_DOWNLOAD was denied.";
		return ERR_VM;
	}

	errmsg = "`upload` was successful.";
	return ERR_UNK;
}
