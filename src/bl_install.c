/**
 * @file bl_install.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief Download new firmware to the brick
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
 * @brief Erase internal flash
 * @retval error according to enum #ERR
 */
static int bootloader_erase(void);

/**
 * @brief Start the programming operation
 * @param offset Start of programmed region
 * @param length Length of programmed region
 * @retval error according to enum #ERR
 */
static int bootloader_start(int offset, int length);

/**
 * @brief Send firmware to bootloader
 * @param fp Firmware file
 * @param length Length of programmed region
 * @param pCrc32 Locally-calculated CRC32 of the firmware
 * @retval error according to enum #ERR
 */
static int bootloader_send(FILE *fp, int length, u32* pCrc32);

/**
 * @brief Request CRC32 verification from the bootloader
 * @param offset Start of programmed region
 * @param length Length of programmed region
 * @param pCrc32 Remotely-calculated CRC32 of the region
 * @retval error according to enum #ERR
 */
static int bootloader_checksum(int offset, int length, u32 *pCrc32);

/* this matches LEGO FW sizes */
#define FLASH_START 0x00000000
#define FLASH_SIZE (16 * 1000 * 1024)

/**
 * @brief Install new firmware binary to the internal flash.
 * @param fp Firmware file to download to the brick.
 * @param offset Start of the region to program
 * @param length Length of the region to program or -1 to autodetect
 * @retval error according to enum #ERR
 */
int bootloader_install(FILE *fp)
{
	int err = ERR_UNK;
	u32 local_crc32 = 0;
	u32 remote_crc32 = 0;

	puts("Erasing flash... (takes 1 minute 48 seconds)"); // 108 seconds
	err = bootloader_erase();
	if (err != ERR_UNK)
		return err;

	puts("Downloading...");
	err = bootloader_start(FLASH_START, FLASH_SIZE); // extremely short
	if (err != ERR_UNK)
		return err;
	err = bootloader_send(fp, FLASH_SIZE, &local_crc32); // variable time
	if (err != ERR_UNK)
		return err;

	puts("Calculating flash checksum... (takes 17 seconds)");
	err = bootloader_checksum(FLASH_START, FLASH_SIZE, &remote_crc32); // 16.7 seconds
	if (err != ERR_UNK)
		return err;

	if (local_crc32 != remote_crc32) {
		fprintf(stderr, "error: checksums do not match: remote %08X != local %08X\n",
				remote_crc32, local_crc32);
		return ERR_IO;
	} else {
		puts("Success! Local and remote checksums match. Rebooting.");
	}

	return bootloader_exit();
}

static int bootloader_erase(void)
{
	return ERR_VM;
}

static int bootloader_start(int offset, int length)
{
	(void) offset, (void) length;
	return ERR_VM;
}

static int bootloader_send(FILE *fp, int length, u32* pCrc32)
{
	(void) fp, (void) length, (void) pCrc32;
	return ERR_VM;
}

static int bootloader_checksum(int offset, int length, u32 *pCrc32)
{
	(void) offset, (void) length, (void) pCrc32;
	return ERR_VM;
}
