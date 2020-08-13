/**
 * @file bl_enter.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief Enter firmware update mode
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
 * @brief Reboot from Linux to bootloader eeprom (firmware update mode).
 *
 * @retval error according to enum #ERR
 */

int bootloader_enter(void)
{
	ENTERFWUPDATE *reboot = packet_alloc(ENTERFWUPDATE, 0);
	int res = ev3_write(handle, (u8 *) reboot, reboot->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write ENTERFWUPDATE.";
		return ERR_COMM;
	}

	/* no reply after this - linux reboots and the connection is gone */

	errmsg = "`ENTERFWUPDATE` was successful.";
	return ERR_UNK;
}
