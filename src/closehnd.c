/**
 * @file closehnd.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief Close a remote file handle
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
 * @brief Close given EV3 file handle range
 * @param [in] start Inclusive start of the range
 * @param [in] end Exclusive end of the range
 * @retval error according to enum #ERR
 */
int closehnd(int start, int end)
{
	CLOSE_HANDLE       *request = packet_alloc(CLOSE_HANDLE, 0);
	CLOSE_HANDLE_REPLY *reply   = malloc(sizeof(CLOSE_HANDLE_REPLY));

	if (start < 0) start = 0;
	if (end > 256) end = 256;

	puts("Handle map:");
	int count = 0;
	for (int hnd = start; hnd < end; hnd++) {
		request->handle = hnd;

		int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
		if (res < 0)
		{
			errmsg = "Unable to write CLOSE_HANDLE.";
			return ERR_COMM;
		}

		res = ev3_read_timeout(handle, (u8 *) reply, sizeof(CLOSE_HANDLE_REPLY), TIMEOUT);
		if (res <= 0)
		{
			errmsg = "Unable to read CLOSE_HANDLE";
			return ERR_COMM;
		}

		count++;

		if (reply->type == VM_OK) // handle was open
			printf("*");
		else // handle was closed
			printf("_");

		if (count % 16 == 0)
			printf("\n");

		fflush(stdout);
	}

	errmsg = "`closehnd` was successful.";
	return ERR_UNK;
}
