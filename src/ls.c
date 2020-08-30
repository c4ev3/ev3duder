/**
 * @file ls.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Lists files on brick
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ev3_io.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

/**
 * @brief List files and directories at \p path. Non-absolute paths are relative to <em>/home/root/lms2012/prjs/sys/</em>
 * @param [in] loc Local FILE*s
 * @param [in] rem Remote EV3 UTF-8 encoded path strings
 *
 * @retval error according to enum #ERR
 * @see http://topikachu.github.io/python-ev3/UIdesign.html
 * @warning ls /proc will lock up the virtual machine (reading from /proc/kmsg locks up the VM).
 * So refrain from doing it or exposing it to the user
 * The Eclipse Plugin handles /proc specially by making it non expandable
 * in the file manager
 */

int ls(const char *path)
{
	int res;
	size_t path_sz = strlen(path) + 1;

	LIST_FILES *list = packet_alloc(LIST_FILES, path_sz);
	memcpy(list->path, path, path_sz);
	list->maxBytes = 1024 - sizeof(LIST_FILES_REPLY);

	//print_bytes(list, list->packetLen + PREFIX_SIZE);
	res = ev3_write(handle, (u8 *) list, list->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write LIST_FILES.";
		return ERR_COMM;
	}
	size_t file_chunksz = 1024;
	void *file_chunk = malloc(file_chunksz);

	LIST_FILES_REPLY *listrep = file_chunk;

	res = ev3_read_timeout(handle, (u8 *) listrep, file_chunksz, TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read LIST_FILES";
		return ERR_COMM;
	}

	if (listrep->type == VM_ERROR)
	{
		errno = listrep->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(listrep, listrep->packetLen);


		errmsg = "`LIST_FILES` was denied.";
		return ERR_VM;
	}
	unsigned read_so_far = listrep->packetLen + 2 - (unsigned) offsetof(LIST_FILES_REPLY, list);
	fwrite(listrep->list, read_so_far, 1, stdout);

	unsigned total = listrep->listSize;

	CONTINUE_LIST_FILES listcon = CONTINUE_LIST_FILES_INIT;
	listcon.handle = listrep->handle;
	listcon.listSize = file_chunksz - sizeof(CONTINUE_LIST_FILES_REPLY);
	//fprintf(stderr, "read %u from total %u bytes.\n", read_so_far, total);
	CONTINUE_LIST_FILES_REPLY *listconrep = file_chunk;
	while (read_so_far < total)
	{
		res = ev3_write(handle, (u8 *) &listcon, sizeof listcon);
		if (res < 0)
		{
			errmsg = "Unable to write CONTINUE_LIST_FILES";
			return ERR_COMM;
		}

		res = ev3_read_timeout(handle, (u8 *) listconrep, file_chunksz, TIMEOUT);
		if (res <= 0)
		{
			errmsg = "Unable to read CONTINUE_LIST_FILES_REPLY";
			return ERR_COMM;
		}
		if (listconrep->type == VM_ERROR)
		{
			errno = listconrep->ret;
			fputs("Operation failed.\nlast_reply=", stderr);
			print_bytes(listconrep, listconrep->packetLen);

			errmsg = "`CONTINUE_LIST_FILES` was denied.";
			return ERR_VM;
		}
		size_t read_this_time = listconrep->packetLen + 2 - offsetof(CONTINUE_LIST_FILES_REPLY, list);
		fwrite(listconrep->list, read_this_time, 1, stdout);
		fflush(stdout);
		listcon.handle = listconrep->handle;
		read_so_far += read_this_time;
		//fprintf(stderr, "read %u from total %u bytes.\n", read_so_far, total);
	}

	errmsg = "`LIST_FILES` was successful.";
	return ERR_UNK;

}
