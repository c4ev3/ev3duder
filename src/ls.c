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

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

/**
 * @brief List files and directories at \p path. path is always relative to <em>/home/root/lms2012/prjs/sys/<em>
 * @param [in] loc Local FILE*s
 * @param [in] rem Remote EV3 UTF-8 encoded path strings
 *
 * @retval error according to enum #ERR
 * @see http://topikachu.github.io/python-ev3/UIdesign.html
 * @warning Doesn't handle replies over 1014 byte in length.
 *      implementation of \p CONTINUTE_LIST_FILES isn't tested
 *		because CONTINUE_LIST_FILES isn't implemented in "firmware".
 * @warning ls /proc will lock up the virtual machine. 
 * So refrain from doing it or exposing it to the user
 * The Eclipse Plugin handles /proc specially by making it non expandable
 * in the file manager 
 */

#define MAX_READ 1024
int ls(const char *path)
{
	int res;
	size_t path_sz = strlen(path) + 1;
	LIST_FILES *list = packet_alloc(LIST_FILES, path_sz);
	list->maxBytes = MAX_READ;
	memcpy(list->path, path, path_sz);

	print_bytes(list, list->packetLen + PREFIX_SIZE);
	res = ev3_write(handle, (u8 *)list, list->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write LIST_FILES.";
		return ERR_COMM;
	}
	fputs("Checking reply: \n", stderr);
	size_t listrep_sz = sizeof(LIST_FILES_REPLY) + list->maxBytes;
	LIST_FILES_REPLY *listrep = calloc(listrep_sz, 1);

	res = ev3_read_timeout(handle, (u8 *)listrep, listrep_sz, TIMEOUT);
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
	printf("listrep->packetLen=%hu, res=%d\n", listrep->packetLen, res);
	//
	fwrite(listrep->list, 1, listrep->packetLen - 10 <= MAX_READ ? listrep->packetLen - 10 : 1024, stdout); // No NUL Termination over Serial COM for whatever reason.
	//
	// Excerpt from the lms2012O sources:  - LIST_FILES should work as long as list does not exceed 1014 bytes. CONTINUE_LISTFILES has NOT been implemented yet.
#if !LEGO_FIXED_CONTINUE_LIST_FILES

#else
	size_t read_so_far = listrep->packetLen + 2 - offsetof(LIST_FILES_REPLY, list);
	size_t total = listrep->listSize;
	CONTINUE_LIST_FILES listcon = CONTINUE_LIST_FILES_INIT;
	listcon.handle =listrep->handle;
	listcon.listSize = 100;
	//	fprintf(stderr, "read %zu from total %zu bytes.\n", read_so_far, total);
	size_t listconrep_sz = sizeof(CONTINUE_LIST_FILES_REPLY) + MAX_READ;
	CONTINUE_LIST_FILES_REPLY *listconrep = malloc(listconrep_sz);
	int ret = listconrep->ret;
	while(ret != END_OF_FILE)
	{
		CONTINUE_LIST_FILES_REPLY *listconrep = malloc(listconrep_sz);
		res = ev3_write(handle, (u8*)&listcon, sizeof listcon);
		if (res < 0)
		{
			errmsg = "Unable to write LIST_FILES";
			return ERR_COMM;
		}

		res = ev3_read_timeout(handle, (u8 *)listconrep, listconrep_sz, TIMEOUT);
		if (res <= 0)
		{
			errmsg = "Unable to read LIST_FILES_REPLY";
			return ERR_COMM;
		}
		if (listconrep->type == VM_ERROR)
		{
			errno = listconrep->ret;
			fputs("Operation failed.\nlast_reply=", stderr);
			print_bytes(listconrep, listconrep->packetLen);


			errmsg = "`LIST_FILES` was denied.";
			return ERR_VM;
		}	
		fprintf(stdout, "%s", listconrep->list);
		listcon.handle = listconrep->handle;
		listcon.listSize = MAX_READ;
		size_t read_so_far = listconrep->packetLen + 2 - offsetof(CONTINUE_LIST_FILES_REPLY, list);
		fflush(stdout);
		//fprintf(stderr, "read %zu from total %zu bytes.\n", read_so_far, listconrep_sz);
		ret = listconrep->ret;
	}
#endif

	errmsg = "`LIST_FILES` was successful.";
	return ERR_UNK;

}

