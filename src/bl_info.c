/**
 * @file bl_info.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief Get version of bootloader EEPROM
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
 * @brief Known hardware revisions
 * See https://www.ev3dev.org/docs/kernel-hackers-notebook/ev3-eeprom/
 */
static const char *hw_names[] = {
	[0] = "unknown",
	[1] = "ONE2ONE prototype",
	[2] = "FINAL prototype",
	[3] = "FINALB/EP1 prototype",
	[4] = "EP2 prototype",
	[5] = "EP3 prototype",
	[6] = "mass produced hardware",
};

/**
 * @brief Get the version of the EEPROM bootloader and the brick hardware.
 *
 * @retval error according to enum #ERR
 */

int bootloader_info(void)
{
	FW_GETVERSION *reboot = packet_alloc(FW_GETVERSION, 0);
	int res = ev3_write(handle, (u8 *) reboot, reboot->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_GETVERSION.";
		return ERR_COMM;
	}

	FW_GETVERSION_REPLY *reply = malloc(sizeof(FW_GETVERSION_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_GETVERSION_REPLY), TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read FW_GETVERSION";
		return ERR_COMM;
	}

	// Looped-back packets (e.g. reply->type == VM_SYS_RQ) are acceptable.
	// According to https://github.com/pybricks/pybricksdev/commit/81f310da54ff40092bb4166226f8d52f943404c5
	// the issue is less severe than originally anticipated. Only a part
	// of the response buffer is overwritten by the PC's request and
	// in this case the EEPROM version is preserved.

	if (reply->type != VM_OK && reply->type != VM_SYS_RQ)
	{
		errno = reply->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen);

		errmsg = "`FW_GETVERSION` was denied.";
		return ERR_VM;
	}

	if (reply->type == VM_OK) {
		int name_idx = 0;
		if (1 <= reply->hardwareVersion && reply->hardwareVersion <= 6)
			name_idx = reply->hardwareVersion;

		printf("Hardware version : V%4.2f (%s)\n", reply->hardwareVersion / 10.0f, hw_names[name_idx]);
		printf("EEPROM version   : V%4.2f\n",      reply->eepromVersion   / 10.0f);
	} else { // VM_SYS_RQ
		// hardwareVersion is corrupted, only eepromVersion is valid.
		// However, in production HW versions it holds that eepromVersion == hardwareVersion
		int name_idx = 0;
		if (1 <= reply->eepromVersion && reply->eepromVersion <= 6)
			name_idx = reply->eepromVersion;

		printf("Hardware version : unknown (unable to query via USB 3.0)\n");
		printf("EEPROM version   : V%4.2f (%s)\n", reply->eepromVersion / 10.0f, hw_names[name_idx]);
	}

	errmsg = "`FW_GETVERSION` was successful.";
	return ERR_UNK;
}
