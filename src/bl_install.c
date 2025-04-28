/**
 * @file bl_install.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief Download new firmware to the brick
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "ev3_io.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

/**
 * @brief Start the programming operation
 * @param offset Start of programmed region
 * @param length Length of programmed region
 * @retval error according to enum #ERR
 */
static int bootloader_erase_and_start(int offset, int length);

/**
 * @brief Send firmware to bootloader
 * @param firmware Firmware image
 * @param length Length of programmed region
 * @param pCrc32 Locally-calculated CRC32 of the firmware
 * @retval error according to enum #ERR
 */
static int bootloader_send(u8 *firmware, int length, u32* pCrc32);

/**
 * @brief Request CRC32 verification from the bootloader
 * @param offset Start of programmed region
 * @param length Length of programmed region
 * @param pCrc32 Remotely-calculated CRC32 of the region
 * @retval error according to enum #ERR
 */
static int bootloader_checksum(int offset, int length, u32 *pCrc32);

/**
 * @brief Install new firmware binary to the internal flash.
 * @param fp Firmware file to download to the brick.
 * @param offset Start of the region to program
 * @param length Length of the region to program or -1 to autodetect
 * @retval error according to enum #ERR
 */
int bootloader_install(FILE *fp)
{
	u8 *firmware = calloc(FLASH_SIZE, 1);
	if (!firmware) {
		errmsg = "out of memory";
		return ERR_NOMEM;
	}

	int read_bytes = fread(firmware, 1, FLASH_SIZE, fp); // this may be less than FLASH_SIZE (e.g. Pybricks has a small firmware image)
	if (ferror(fp)) {
		errmsg = "Reading of firmware file failed";
		free(firmware);
		return ERR_IO;
	}

	int sector_count = (read_bytes + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;

	printf("Programming %d EV3 flash sectors...\n", sector_count);

	int err = ERR_UNK;
	for (int sector = 0; sector < sector_count; sector++) {
		u32 local_crc32 = 0;
		u32 remote_crc32 = 0;

		printf("- Sector %3d (%2d %%)\n", sector, 100*sector/sector_count);
		err = bootloader_erase_and_start(sector*FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
		if (err != ERR_UNK) {
			puts("Flash erase returned an error!");
			break;
		}

		err = bootloader_send(firmware + sector*FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE, &local_crc32);
		if (err != ERR_UNK) {
			puts("Programming returned an error!");
			break;
		}

		err = bootloader_checksum(sector*FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE, &remote_crc32);
		if (err == ERR_USBLOOP && sector == 0)
		{
			puts("NOTE: CRC not checked, because the brick is likely plugged to a USB 3.0 port.");
		}
		else if (err == ERR_UNK) // no error occurred
		{
			if (local_crc32 != remote_crc32) {
				printf("Checksum does not match: remote %08X != local %08X\n", remote_crc32, local_crc32);
				err = ERR_COMM;
				break;
			}
		}
		else // other error occurred
		{
			puts("Checksum computation returned an error!");
			break;
		}
	}

	free(firmware);
	if (err == ERR_UNK) {
		puts("Flashing finished, rebooting the brick.");
		return bootloader_exit();
	} else {
		puts("Some error occurred, leaving the brick in the bootloader mode.");
		puts("To exit it manually, simply remove the EV3 battery and then put it back in.");
		return err;
	}
}

static int bootloader_erase_and_start(int offset, int length)
{
	FW_START_DOWNLOAD_WITH_ERASE *request = NULL;
	FW_START_DOWNLOAD_WITH_ERASE_REPLY *reply = NULL;
	int err;

	request = packet_alloc(FW_START_DOWNLOAD_WITH_ERASE, 0);
	request->flashStart = offset;
	request->flashLength = length;
	int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_START_DOWNLOAD.";
		err = ERR_COMM;
		goto exit;
	}

	reply = malloc(sizeof(FW_START_DOWNLOAD_WITH_ERASE_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_START_DOWNLOAD_WITH_ERASE_REPLY), -1);
	if (res <= 0)
	{
		errmsg = "Unable to read FW_START_DOWNLOAD";
		err = ERR_COMM;
		goto exit;
	}

	// note: accept looped-back packets (usb 3.0 bug; reply not required here)
	if (reply->type != VM_OK && reply->type != VM_SYS_RQ)
	{
		errno = reply->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen + 2);

		errmsg = "`FW_START_DOWNLOAD_WITH_ERASE_REPLY` was denied.";
		err = ERR_VM;
		goto exit;
	}
	err = ERR_UNK;

exit:
	free(request);
	free(reply);
	return err;
}

static int bootloader_send(u8 *buffer, int length, u32* pCrc32)
{
	const int max_payload = 1024 - (sizeof(FW_DOWNLOAD_DATA) - PREFIX_SIZE + 2);
	FW_DOWNLOAD_DATA *request = packet_alloc(FW_DOWNLOAD_DATA, max_payload);
	FW_DOWNLOAD_DATA_REPLY *reply = malloc(sizeof(FW_DOWNLOAD_DATA_REPLY));

	int total = length;
	int sent_so_far = 0;
	int err = ERR_UNK;
	u32 crc = 0;

	while (sent_so_far < total) {
		int remaining = total - sent_so_far;
		int this_block = remaining <= max_payload ? remaining : max_payload;
		int res;

		memcpy(request->payload, buffer + sent_so_far, this_block);
		crc = crc32(crc, request->payload, this_block);

		request->packetLen = sizeof(FW_DOWNLOAD_DATA) - PREFIX_SIZE + this_block;

		res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
		if (res < 0)
		{
			errmsg = "Unable to write FW_DOWNLOAD_DATA.";
			err = ERR_COMM;
			break;
		}

		res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_DOWNLOAD_DATA_REPLY), -1);
		if (res <= 0)
		{
			errmsg = "Unable to read FW_DOWNLOAD_DATA";
			err = ERR_COMM;
			break;
		}

		// note: accept looped-back packets (usb 3.0 bug; reply not required here)
		if (reply->type != VM_OK && reply->type != VM_SYS_RQ)
		{
			errno = reply->ret;
			fputs("Operation failed.\nlast_reply=", stderr);
			print_bytes(reply, reply->packetLen + 2);

			errmsg = "`FW_DOWNLOAD_DATA` was denied.";
			err = ERR_VM;
			break;
		}

		sent_so_far += this_block;
	}

	*pCrc32 = crc;
	free(request);
	free(reply);
	return err;
}

/**
 * @brief Compare CRCs of the flash memory and the firmware image.
 * @param fp Firmware file that is supposed to be flashed in the brick.
 * @param starting_sector First 64 KiB sector of the flash memory to check (0-based)
 * @param num_sectors Number of 64 KiB sectors to check
 * @param verbose If true, CRC is checked for each sector individually.
 *                Otherwise, it is computed across all sectors at once.
 * @retval error according to enum #ERR
 */
int bootloader_crc(FILE *fp, u32 starting_sector, u32 num_sectors, bool verbose)
{
	u8 *firmware = calloc(FLASH_SIZE, 1);
	if (!firmware) {
		errmsg = "out of memory";
		return ERR_NOMEM;
	}
	u32 read_bytes = fread(firmware, 1, FLASH_SIZE, fp);
	if (ferror(fp)) {
		errmsg = "Reading of firmware file failed";
		free(firmware);
		return ERR_IO;
	}
	u32 wanted_bytes = (starting_sector + num_sectors) * FLASH_SECTOR_SIZE;
	if (read_bytes < wanted_bytes) {
		printf("WARNING: firmware file is truncated: %d bytes expected, %d bytes read\n", wanted_bytes, read_bytes);
	}

	int err = ERR_UNK;
	if (verbose) {
		// Sector-by-sector CRC comparison.
		// Compares the remote and local CRCs of each sector, showing the results of each.
		printf("Sector CRC comparison:\n");
		bool all_ok = true;
		for (u32 sector = starting_sector; sector < starting_sector + num_sectors; sector++) {
			u32 local_sector_crc32 = crc32(0, firmware + sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
			u32 remote_sector_crc32 = 0;
			int sector_err = bootloader_checksum(sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE, &remote_sector_crc32);

			bool this_ok = remote_sector_crc32 == local_sector_crc32;
			all_ok = all_ok && this_ok;

			if (sector_err == ERR_UNK) {
				printf("Sector %3d: Remote CRC = %08X, local CRC = %08X%s\n", sector, remote_sector_crc32,
					local_sector_crc32, this_ok ? "" : " ERR");
			} else {
				printf("Error requesting sector CRC.  Err = %i\n", sector_err);
				err = sector_err;
			}
		}

		if (all_ok) {
			puts("All sectors match.");
		} else {
			puts("Some sectors do not match.");
		}
	} else {
		// Do a single comparison, calculating the CRC over multiple sectors.
		u32 local_crc32 = crc32(0, firmware + starting_sector * FLASH_SECTOR_SIZE, num_sectors * FLASH_SECTOR_SIZE);
		printf("Requesting remote CRC...\n");
		u32 remote_crc32 = 0;
		err = bootloader_checksum(starting_sector * FLASH_SECTOR_SIZE, num_sectors * FLASH_SECTOR_SIZE, &remote_crc32);
		if (err == ERR_UNK) {
			printf("Remote CRC = %08X, local CRC = %08X%s\n", remote_crc32, local_crc32,
				remote_crc32 == local_crc32 ? "" : " ERR");
		} else {
			printf("Error requesting full CRC.  Err = %i\n", err);
		}
	}

	free(firmware);
	return err;
}

static int bootloader_checksum(int offset, int length, u32 *pCrc32)
{
	FW_GETCRC32 *request = NULL;
	FW_GETCRC32_REPLY *reply = NULL;
	int err;

	*pCrc32 = 0;

	request = packet_alloc(FW_GETCRC32, 0);
	request->flashStart = offset;
	request->flashLength = length;
	int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_GETCRC32.";
		err = ERR_COMM;
		goto exit;
	}

	reply = malloc(sizeof(FW_GETCRC32_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_GETCRC32_REPLY), -1);
	if (res <= 0)
	{
		errmsg = "Unable to read FW_GETCRC32";
		err = ERR_COMM;
		goto exit;
	}

	// note: report loopback bug to outer code
	if (reply->type == VM_SYS_RQ)
	{
		err = ERR_USBLOOP;
		goto exit;
	}

	if (reply->type != VM_OK)
	{
		errno = reply->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen + 2);

		errmsg = "`FW_GETCRC32` was denied.";
		err = ERR_VM;
		goto exit;
	}
	*pCrc32 = reply->crc32;
	err = ERR_UNK;

exit:
	free(request);
	free(reply);
	return err;
}
