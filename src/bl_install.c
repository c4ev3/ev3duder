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

/* this matches LEGO FW sizes */
#define FLASH_START 0x00000000
#define FLASH_SIZE (16 * 1000 * 1024)
#define FLASH_SECTOR (64*1024) // N25Q128 datasheet says that it has 64-Kbyte sectors/eraseblocks

/**
 * @brief Install new firmware binary to the internal flash.
 * @param fp Firmware file to download to the brick.
 * @param offset Start of the region to program
 * @param length Length of the region to program or -1 to autodetect
 * @retval error according to enum #ERR
 */
int bootloader_install(FILE *fp)
{
	// leaking memory is not ideal, but the OS will handle it somewhat
	u8 *firmware = calloc(FLASH_SIZE, 1);
	int read_bytes = fread(firmware, 1, FLASH_SIZE, fp);
	if (read_bytes != FLASH_SIZE) {
		printf("WARNING: firmware file might be truncated: %d bytes expected, %d bytes read\n", FLASH_SIZE, read_bytes);
	}


	for (int sector = 0; sector < FLASH_SIZE/FLASH_SECTOR; sector++) {
		int err = ERR_UNK;
		u32 local_crc32 = 0;
		u32 remote_crc32 = 0;

		printf("Programming sector %d/%d...\n", sector+1, FLASH_SIZE/FLASH_SECTOR);
		err = bootloader_erase_and_start(sector*FLASH_SECTOR, FLASH_SECTOR);
		if (err != ERR_UNK) {
			puts("ERASE FAILED, continuing");
			continue;
		}

		err = bootloader_send(firmware + sector*FLASH_SECTOR, FLASH_SECTOR, &local_crc32);
		if (err != ERR_UNK) {
			puts("PROGRAMMING FAILED, continuing");
			continue;
		}

		err = bootloader_checksum(sector*FLASH_SECTOR, FLASH_SECTOR, &remote_crc32);
		if (err == ERR_USBLOOP)
		{
			puts("WARNING: CRC not checked, because the brick is likely plugged to a USB 3.0 port.");
		}
		else if (err == ERR_UNK) // no error occurred
		{
			if (local_crc32 != remote_crc32) {
				printf("CHECKSUM MISMATCH: remote %08X != local %08X, continuing\n", remote_crc32, local_crc32);
				continue;
			}
		}
		else // other error occurred
		{
			puts("CRC COMPUTATION FAILED, continuing");
			continue;
		}
	}

	puts("Flashing finished, rebooting the brick.");

	return bootloader_exit();
}

static int bootloader_erase_and_start(int offset, int length)
{
	FW_START_DOWNLOAD_WITH_ERASE *request = packet_alloc(FW_START_DOWNLOAD_WITH_ERASE, 0);
	request->flashStart = offset;
	request->flashLength = length;
	int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write FW_START_DOWNLOAD.";
		return ERR_COMM;
	}

	FW_START_DOWNLOAD_WITH_ERASE_REPLY *reply = malloc(sizeof(FW_START_DOWNLOAD_WITH_ERASE_REPLY));
	res = ev3_read_timeout(handle, (u8 *) reply, sizeof(FW_START_DOWNLOAD_WITH_ERASE_REPLY), -1);
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

		errmsg = "`FW_START_DOWNLOAD_WITH_ERASE_REPLY` was denied.";
		return ERR_VM;
	}
	return ERR_UNK;
}

static int bootloader_send(u8 *buffer, int length, u32* pCrc32)
{
	*pCrc32 = 0;

	int max_payload = 1024 - (sizeof(FW_DOWNLOAD_DATA) - PREFIX_SIZE + 2);
	FW_DOWNLOAD_DATA *request = packet_alloc(FW_DOWNLOAD_DATA, max_payload);
	FW_DOWNLOAD_DATA_REPLY *reply = malloc(sizeof(FW_DOWNLOAD_DATA_REPLY));

	int total = length;
	int sent_so_far = 0;
	int res = 0;
	u32 crc = 0;

	while (sent_so_far < total) {
		int remaining = total - sent_so_far;
		int this_block = remaining <= max_payload ? remaining : max_payload;

		memcpy(request->payload, buffer+sent_so_far, this_block);
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
	}
	*pCrc32 = crc;
	return ERR_UNK;
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
