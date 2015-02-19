#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>

#include "typedefs.h"
#include "systemcmd.h"

//define DEBUG

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

#define VendorID 0x694 /* LEGO GROUP */
#define ProductID 0x005 /* EV3 */
#define SerialID NULL
#define TIMEOUT 2000 /* in milli seconds */


#define MAX_STR 256
#define CHUNK_SIZE 1000 //FIXME: for some reason HIDAPI refuses sending more bytes?!
//(((u16)-1 - sizeof (CONTINUE_DOWNLOAD) - PREFIX_SIZE)/1)

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
	const char *src, *dst;
	BEGIN_DOWNLOAD_REPLY bdrep;

	if (argc > 2)
	{
		src = argv[1];
		dst = argv[2];
	}
	else // defaults to
	{
		dst = "../apps/a3f/a3f.rbf";
		src = "hamada.rbf";
	}

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

	printf("Attempting file upload (%ldb total; %u chunks): ", fsize, extra_chunks + 1);
	fseek(fp, 0, SEEK_SET);



	CONTINUE_DOWNLOAD **cd = malloc((1 + extra_chunks) * sizeof(*cd));
	{
		int i = 0;
		for (; i < extra_chunks; ++i)
		{
			cd[i] = packet_alloc(CONTINUE_DOWNLOAD, CHUNK_SIZE);
			fread(cd[i]->fileChunk, CHUNK_SIZE, 1, fp);
		}

		cd[i] = packet_alloc(CONTINUE_DOWNLOAD, final_chunk_sz);

		fread(cd[i]->fileChunk, final_chunk_sz, 1, fp);
		fclose(fp);
	}

	//TODO: read in chunks, whatif long isnt big enough
	BEGIN_DOWNLOAD *bd = packet_alloc(BEGIN_DOWNLOAD, strlen(dst) + 1);
	bd->fileSize = fsize;
	strcpy(bd->fileName, dst);

#ifdef DEBUG
	print_bytes(bd, len);
#endif

	res = hid_write(handle, (u8 *)bd, bd->packetLen + PREFIX_SIZE);
	if (res < 0)
		die("Unable to write BEGIN_DOWNLOAD.");

	fputs("Checking reply: \n", stdout);


	res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
	if (res == 0)
		die("Request timed out.");
	if (res < 0)
		die("Unable to read.");

#ifdef DEBUG
	putchar(">");
	print_bytes(&bdrep, sizeof(bdrep));
	putchar('\n');
	printf("current handle is %u\n", bdrep.fileHandle);
#endif

	for (size_t j = 0; j <= extra_chunks; ++j)
	{
		cd[j]->fileHandle = bdrep.fileHandle;
		res = hid_write(handle, (u8 *)cd[j], cd[j]->packetLen + PREFIX_SIZE);
		if (res < 0)
			die("Unable to write CONTINUE_DOWNLOAD.");
	}
	res = hid_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
	if (res == 0)
		die("Request timed out.");
	if (res < 0)
		die("Unable to read.");

	if (memcmp(&bdrep, &CONTINUE_DOWNLOAD_REPLY_SUCCESS, sizeof bdrep - 3) == 0
	        && (bdrep.ret == 0 || bdrep.ret == 8) //README: why sometimes EOF and others SUCCESS?
	   )
		printf("Transfer has been successful! (ret=%d)", bdrep.ret);
	else
	{
		fputs("Transfer failed.\nlast_reply=", stderr);
		print_bytes(&bdrep, sizeof(bdrep));
		return __LINE__;
	}

	return 0;
}


