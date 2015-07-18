/**
 * @file mkdir.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 2.0
 * @brief creates a directory structure on the ev3
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
 * @brief creates directory structures relative to <em>/home/root/lms2012/prjs/sys/<em>
 * @param [in] path directory structure
 *
 * @retval error according to enum #ERR
 * @see your man pages for \p mkdir
 * @note The VM will choke to death when you excede the ASCII subset of UTF-8
 *		Do NOT use any non ASCII letters if you want the LEGO menu to stay responsible.
 *		In case of fudging up, \p ev3duder rm() can delete directories too
 */
int mkdir(const char *path)
{
	int res;
	size_t path_sz = strlen(path) + 1;
	CREATE_DIR *mk = packet_alloc(CREATE_DIR, path_sz);
	memcpy(mk->path, path, path_sz);

	res = ev3_write(handle, (u8 *)mk, mk->packetLen + PREFIX_SIZE);
	if (res < 0) {
		errmsg = "Unable to write CREATE_DIR.";
		return ERR_COMM;
	}
	fputs("Checking reply: \n", stderr);
	CREATE_DIR_REPLY mkrep;

	res = ev3_read_timeout(handle, (u8 *)&mkrep, sizeof mkrep, TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read CREATE_DIR_REPLY";
		return ERR_COMM;
	}

	if (mkrep.type == VM_ERROR)
	{
		errno = mkrep.ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(&mkrep, mkrep.packetLen);


		errmsg = "`CREATE_DIR` was denied.";
		return ERR_VM;
	}

	errmsg = "`CREATE_DIR` was successful.";
	return ERR_UNK;
}

