#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hidapi.h>

#include "typedefs.h"
#include "systemcmd.h"

#define DEBUG 3

#define print_bytes(buf, len) \
	do {\
		u8 *ptr = (u8*)buf;\
		for (int i = 0; i != len; i++, ptr++) printf("%02x ", *ptr);\
	} while (0*putchar('\n'))


#define die(msg) \
	do {\
		fprintf(stderr, "[%s:%d]: %ls (%s)", __func__, __LINE__, hid_error(handle), msg);\
		exit(__LINE__);\
	} while (0)

#ifndef _GNU_SOURCE
#define mempcpy(dst, src, len) \
	(memcpy(dst, src, len) + len)
#endif

#define VendorID 0x694 /* LEGO GROUP */
#define ProductID 0x005 /* EV3 */
#define SerialID NULL
#define TIMEOUT 2000 /* in milli seconds */


#define MAX_STR 256
#define CHUNK_SIZE 1000 //FIXME: for some reason HIDAPI refuses sending more bytes?!
//(((u16)~0 - sizeof (CONTINUE_DOWNLOAD) - PREFIX_SIZE)/1)

#define packet_alloc(type, extra) _packet_alloc(sizeof(type), extra, &type##_INIT)
static void *_packet_alloc(size_t size, size_t extra, void *init)
{
	void *ptr = malloc(size + extra);
	memcpy(ptr, init, size);
	((SYSTEM_CMD *)ptr)->packetLen = size + extra - PREFIX_SIZE;
	return ptr;
}

static const u8 tone[] =
{0x0, 0x0F, 0x00, 0, 0, 0x80, 0x00, 0x00, 0x94, 0x01, 0x81, 0x02, 0x82, 0xE8, 0x03, 0x82, 0xE8, 0x03};

int main(int argc, char *argv[])
{
	if (!( argc == 3 || (argc == 4 && argv[1][0] == '-' && argv[1][1] == 'e')))
	{
		fputs("USAGE: ev3_cp [-e] <pc_path> <ev3_path>\n", stderr);
		return __LINE__;
	}
	const char *src, *dst;

	bool eval = argc == 4;
	dst = argv[--argc];
	src = argv[--argc];

	int res;
	wchar_t wstr[MAX_STR];


	hid_device *handle = hid_open(VendorID, ProductID, SerialID);
	if (!handle)
	{
		fputs("EV3 not found. Is it properly plugged into the USB port?", stderr);
		return __LINE__;
	}

	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	printf("Manufacturer String: %ls\n", wstr);
	res = hid_get_product_string(handle, wstr, MAX_STR);
	printf("Product String: %ls\n", wstr);
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	printf("Serial Number String: %ls", wstr);
	puts("\n");

	res = hid_write(handle, tone, sizeof tone);
	if (res < 0)
		die("Unable to beep");


	FILE *fp = fopen(src, "rb");
	if (!fp)
	{
		fprintf(stderr, "File <%s> doesn't exist.", src);
		return __LINE__;
	}

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp); // file size limit is min(LONG_MAX, 4gb)
	size_t extra_chunks = fsize / CHUNK_SIZE;
	size_t final_chunk_sz = fsize % CHUNK_SIZE;

	printf("Attempting file upload (%ldb total; %u chunks): \n", fsize, extra_chunks + 1);
	fseek(fp, 0, SEEK_SET);



	CONTINUE_DOWNLOAD **cd = malloc((1 + extra_chunks) * sizeof(*cd));
	{
		size_t ret;
		size_t i = 0;
		for (; i < extra_chunks; ++i)
		{
			cd[i] = packet_alloc(CONTINUE_DOWNLOAD, CHUNK_SIZE);
			ret = fread(cd[i]->fileChunk, 1, CHUNK_SIZE, fp);
#if DEBUG > 2
			printf("%d: %u bytes read.\n", i, ret);
#endif
		}

		cd[i] = packet_alloc(CONTINUE_DOWNLOAD, final_chunk_sz);

		ret = fread(cd[i]->fileChunk, 1, final_chunk_sz, fp);
		printf("%d: %u bytes read.\n", i, ret);
		fclose(fp);
	}

	//TODO: read in chunks, whatif long isnt big enough
	BEGIN_DOWNLOAD *bd = packet_alloc(BEGIN_DOWNLOAD, strlen(dst) + 1);
	bd->fileSize = fsize;
	strcpy(bd->fileName, dst);

#ifdef DEBUG
	puts("=BEGIN_DOWNLOAD");
	print_bytes(bd, PREFIX_SIZE + bd->packetLen);
	puts("=cut");
#endif


	
	res = hid_write(handle, (u8 *)bd, bd->packetLen + PREFIX_SIZE);
	if (res < 0)
		die("Unable to write BEGIN_DOWNLOAD.");

	fputs("Checking reply: \n", stdout);

	BEGIN_DOWNLOAD_REPLY bdrep;

	res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
	if (res == 0)
		die("Request timed out.");
	if (res < 0)
		die("Unable to read.");

	switch (bdrep.ret)
	{
	case 0x0:
		break;
	case 0x9:
		fprintf(stderr, "Does the ev3 path exist? ");
	default:
		fprintf(stderr, "(BEGIN_DOWNLOAD was refused [ret=%u])\n", bdrep.ret);
#ifdef DEBUG
	puts("=BEGIN_DOWNLOAD_REPLY");
	print_bytes(&bdrep, PREFIX_SIZE + bdrep.packetLen);
	puts("=cut");
#endif
		return __LINE__;
	}


#ifdef DEBUG
	puts("=BEGIN_DOWNLOAD_REPLY");
	print_bytes(&bdrep, sizeof(bdrep));
	putchar('\n');
	printf("current handle is %u\n", bdrep.fileHandle);
	puts("=cut");
#endif

	for (size_t i = 0; i <= extra_chunks; ++i)
	{
		cd[i]->fileHandle = bdrep.fileHandle;
		res = hid_write(handle, (u8 *)cd[i], cd[i]->packetLen + PREFIX_SIZE);
#if DEBUG > 7
		printf("=CONTINUE_DOWNLOAD");;
		print_bytes(cd[i], cd[i]->packetLen + PREFIX_SIZE);
		puts("=cut");
#endif
		if (res < 0)
			die("Unable to write CONTINUE_DOWNLOAD.");
#if DEBUG > 1
		res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
		if (res == 0)
			die("Request timed out.");
		if (res < 0)
			die("Unable to read.");

		printf("=CONTINUE_DOWNLOAD_REPLY(chunk=%u, data=%ub)\n", i,
		       cd[i]->packetLen - (sizeof(CONTINUE_DOWNLOAD) - PREFIX_SIZE));
		print_bytes(&bdrep, PREFIX_SIZE + bdrep.packetLen);
		puts("=cut");
	}
#else

	}
	res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
	if (res == 0)
		die("Request timed out.");
	if (res < 0)
		die("Unable to read.");
#endif
	if (memcmp(&bdrep, &CONTINUE_DOWNLOAD_REPLY_SUCCESS, sizeof bdrep - 3) == 0
	        && (bdrep.ret == 0 || bdrep.ret == 8) //README: why sometimes EOF and others SUCCESS?
	   )
		printf("Transfer has been successful! (ret=%d)\n", bdrep.ret);
	else
	{
		fputs("Transfer failed.\nlast_reply=", stderr);
#if DEBUG < 2
	}
	{
#endif

		print_bytes(&bdrep, bdrep.packetLen);
		return __LINE__;
	}

	if (!eval) return 1;

	const char run1[] = {0xC0, 0x08, 0x82, 0x01, 0x00, 0x84};
	const char run2[] = {0x60, 0x64, 0x03, 0x01, 0x60, 0x64, 0x00};

	size_t dst_sz = strlen(dst) + 1;

	EXECUTE_FILE *run = packet_alloc(EXECUTE_FILE, sizeof (run1) + dst_sz + sizeof (run2));

	mempcpy(mempcpy(mempcpy((u8 *)&run->bytes, // run->bytes = [run1] + [dst] + [run2]
	                        run1, sizeof run1),
	                dst,  dst_sz),
	        run2, sizeof run2);

	res = hid_write(handle, (u8 *)run, run->packetLen + PREFIX_SIZE);

	if (res < 0)
		die("Unable to write EXECUTE_FILE.");

	VM_REPLY efrep;

	res = hid_read_timeout(handle, (u8 *)&efrep, sizeof efrep + 2, TIMEOUT);
	if (res == 0)
		die("Request timed out.");
	if (res < 0)
		die("Unable to read.");

	if (memcmp(&efrep, &EXECUTE_FILE_REPLY_SUCCESS, sizeof efrep) == 0)
		printf("Program successfully started! (ret=%d)\n", efrep.bytes[0]);
	else
	{
		fputs("Starting program failed.\nlast_reply=", stderr);
		print_bytes(&efrep, 2 + 2 + 3);
		return __LINE__;
	}

	return 0;
}
