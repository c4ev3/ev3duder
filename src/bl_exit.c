/**
 * @file bl_exit.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief Exit firmware update mode
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
 * @brief Reboot from bootloader eeprom (firmware update mode) to Linux.
 *
 * @retval error according to enum #ERR
 */

int bootloader_exit(void)
{
	FW_STARTAPP *reboot = packet_alloc(FW_STARTAPP, 0);
	int res = ev3_write(handle, (u8 *) reboot, reboot->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_STARTAPP.";
		return ERR_COMM;
	}

	FW_STARTAPP_REPLY *reply = malloc(sizeof(FW_STARTAPP_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_STARTAPP_REPLY), TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read FW_STARTAPP";
		return ERR_COMM;
	}

	if (reply->type != VM_OK)
	{
		errno = reply->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen);

		errmsg = "`FW_STARTAPP` was denied.";
		return ERR_VM;
	}

	errmsg = "`FW_STARTAPP` was successful.";
	return ERR_UNK;
}
