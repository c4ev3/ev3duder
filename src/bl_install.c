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

	// handle usb 3.0 bug
	if (err == ERR_USBLOOP)
	{
		puts("Checksum not checked, assuming update was OK. Rebooting.");
	}
	else if (err == ERR_UNK) // no error occurred
	{
		if (local_crc32 != remote_crc32) {
			fprintf(stderr, "error: checksums do not match: remote %08X != local %08X\n",
					remote_crc32, local_crc32);
			return ERR_IO;
		} else {
			puts("Success! Local and remote checksums match. Rebooting.");
		}
	}
	else // other error occurred
	{
		return err;
	}

	return bootloader_exit();
}

static int bootloader_erase(void)
{
	FW_ERASEFLASH *request = packet_alloc(FW_ERASEFLASH, 0);
	int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_ERASEFLASH.";
		return ERR_COMM;
	}

	FW_ERASEFLASH_REPLY *reply = malloc(sizeof(FW_ERASEFLASH_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_ERASEFLASH_REPLY), -1);
	if (res <= 0)
	{
		errmsg = "Unable to read FW_ERASEFLASH";
		return ERR_COMM;
	}

	// note: accept looped-back packets (usb 3.0 bug; reply not required here)
	if (reply->type != VM_OK && reply->type != VM_SYS_RQ)
	{
		errno = reply->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen + 2);

		errmsg = "`FW_ERASEFLASH` was denied.";
		return ERR_VM;
	}
	return ERR_UNK;
}

static int bootloader_start(int offset, int length)
{
	FW_START_DOWNLOAD *request = packet_alloc(FW_START_DOWNLOAD, 0);
	request->flashStart = offset;
	request->flashLength = length;
	int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_START_DOWNLOAD.";
		return ERR_COMM;
	}

	FW_START_DOWNLOAD_REPLY *reply = malloc(sizeof(FW_START_DOWNLOAD_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_START_DOWNLOAD_REPLY), -1);
	if (res <= 0)
	{
		errmsg = "Unable to read FW_START_DOWNLOAD";
		return ERR_COMM;
	}

	// note: accept looped-back packets (usb 3.0 bug; reply not required here)
	if (reply->type != VM_OK && reply->type != VM_SYS_RQ)
	{
		errno = reply->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen + 2);

		errmsg = "`FW_START_DOWNLOAD` was denied.";
		return ERR_VM;
	}
	return ERR_UNK;
}

static int bootloader_send(FILE *fp, int length, u32* pCrc32)
{
	*pCrc32 = 0;

	int max_payload = 1024 - (sizeof(FW_DOWNLOAD_DATA) - PREFIX_SIZE + 2);
	FW_DOWNLOAD_DATA *request = packet_alloc(FW_DOWNLOAD_DATA, max_payload);
	FW_DOWNLOAD_DATA_REPLY *reply = malloc(sizeof(FW_DOWNLOAD_DATA_REPLY));

	int total = length;
	int sent_so_far = 0;
	int file_ended = 0;
	int res = 0;
	int last_percent = 0;
	u32 crc = 0;

	printf("Progress: %3d %%\n", 0);

	while (sent_so_far < total) {
		int remaining = total - sent_so_far;
		int this_block = remaining <= max_payload ? remaining : max_payload;

		if (!file_ended) {
			int real = fread(request->payload, 1, this_block, fp);
			if (real < this_block) {
				file_ended = 1;
				memset(request->payload + real, 0, this_block - real);
			}
		} else {
			memset(request->payload, 0, this_block);
		}

		crc = crc32(crc, request->payload, this_block);

		request->packetLen = sizeof(FW_DOWNLOAD_DATA) - PREFIX_SIZE + this_block;

		res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
		if (res < 0)
		{
			errmsg = "Unable to write FW_DOWNLOAD_DATA.";
			return ERR_COMM;
		}

		res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_DOWNLOAD_DATA_REPLY), -1);
		if (res <= 0)
		{
			errmsg = "Unable to read FW_DOWNLOAD_DATA";
			return ERR_COMM;
		}

		// note: accept looped-back packets (usb 3.0 bug; reply not required here)
		if (reply->type != VM_OK && reply->type != VM_SYS_RQ)
		{
			errno = reply->ret;
			fputs("Operation failed.\nlast_reply=", stderr);
			print_bytes(reply, reply->packetLen + 2);

			errmsg = "`FW_DOWNLOAD_DATA` was denied.";
			return ERR_VM;
		}

		sent_so_far += this_block;

		int new_percent = sent_so_far * 100 / total;
		if (new_percent >= last_percent + 5) {
			last_percent += ((new_percent - last_percent) / 5) * 5;
			printf("Progress: %3d %%\n", last_percent);
		}
	}
	*pCrc32 = crc;
	return ERR_UNK;
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
	*pCrc32 = 0;

	FW_GETCRC32 *request = packet_alloc(FW_GETCRC32, 0);
	request->flashStart = offset;
	request->flashLength = length;
	int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_GETCRC32.";
		return ERR_COMM;
	}

	FW_GETCRC32_REPLY *reply = malloc(sizeof(FW_GETCRC32_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_GETCRC32_REPLY), -1);
	if (res <= 0)
	{
		errmsg = "Unable to read FW_GETCRC32";
		return ERR_COMM;
	}

	// note: report loopback bug to outer code
	if (reply->type == VM_SYS_RQ)
	{
		return ERR_USBLOOP;
	}

	if (reply->type != VM_OK)
	{
		errno = reply->ret;
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen + 2);

		errmsg = "`FW_GETCRC32` was denied.";
		return ERR_VM;
	}
	*pCrc32 = reply->crc32;
	return ERR_UNK;
}
