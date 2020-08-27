/**
 * @file uf2_pack.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief pack files into microsoft uf2 container (needed for using the usb flash emulation)
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
#include "uf2.h"

// https://stackoverflow.com/a/26359433
#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#ifdef __WIN32
#define BACKSLASH 1
#else
#define BACKSLASH 0
#endif

static int uf2_write_all(FILE *dst_fp, const char *destdir, const char **file_array, int files);
static int uf2_write_file(FILE *dst, FILE *src, const char *filename, int start_block, long file_bytes);
static int uf2_fixup_block_count(FILE *dst_fp, long start, int blocks);

/**
 * @brief Pack files into a UF2 file
 * UF2 specification is available at https://github.com/microsoft/uf2/blob/master/README.md
 * @param [in] fp UF2 output
 * @param [in] brickdir On-brick directory (Projects, USB Stick, SD Card, Temporary Logs, Permanent Logs)
 * @param [in] file_array list of files to insert
 * @param [in] files number of files to pack
 * @retval error according to enum #ERR
 * @bug might not handle files bigger than 2gb :-)
 */
int uf2_pack(FILE *fp, const char *brickdir, const char **file_array, int files)
{
	if (strlen(brickdir) == 0)
		brickdir = "Projects";

	int blocks = uf2_write_all(fp, brickdir, file_array, files);
	if (blocks < 0)
	{
		perror("unexpected error when writing UF2 file");
		errmsg = "`uf2 pack` has failed.";
		return -blocks;
	}

	errmsg = "`uf2 pack` was successful.";
	return ERR_UNK;
}

/**
 * @brief Write UF2 block stream into a given file.
 * @param [in] dst_fp Destination UF2 stream
 * @param [in] destdir Target UF2 directory
 * @param [in] file_array Array of input file paths
 * @param [in] files Number of file names in file_array
 * @retval Number of written UF2 blocks
 */
int uf2_write_all(FILE *dst_fp, const char *destdir, const char **file_array, int files)
{
	char uf2_name[UF2_FILENAME_MAX + 1];
	int blks = 0;
	long p0, p1;

	long start = ftell(dst_fp);
	if (start < 0)
		return -ERR_IO;

	for (int i = 0; i < files; i++)
	{
		snprintf(uf2_name, UF2_FILENAME_MAX + 1, "%s/%s", destdir, uf2_basename(file_array[i], BACKSLASH));
		uf2_name[UF2_FILENAME_MAX] = '\0'; // good ol' Windows

		FILE *src_fp = fopen(file_array[i], "rb");
		if (!src_fp) return -ERR_IO;

		if ((p0 = ftell(src_fp)) < 0)               return -ERR_IO;
		if (      fseek(src_fp, 0, SEEK_END) != 0)  return -ERR_IO;
		if ((p1 = ftell(src_fp)) < 0)               return -ERR_IO;
		if (      fseek(src_fp, p0, SEEK_SET) != 0) return -ERR_IO;

		int ret = uf2_write_file(dst_fp, src_fp, uf2_name, blks, p1 - p0);
		fclose(src_fp);

		if (ret < 0)
			return ret;
		else
			blks += ret;
	}
	
	return uf2_fixup_block_count(dst_fp, start, blks);
}

/**
 * @brief Write a file into a UF2 stream
 * @param [in] dst_fp Destination UF2 stream
 * @param [in] src_fp Source file
 * @param [in] filename UF2 destination path
 * @param [in] start_block Current UF2 block number
 * @param [in] file_bytes How many bytes the file contains
 * @retval Number of blocks taken by this file
 */
int uf2_write_file(FILE *dst_fp, FILE *src_fp, const char *filename, int start_block, long file_bytes)
{
	uf2_block_t block = {
		.magic1       = UF2_MAGIC_1,
		.magic2       = UF2_MAGIC_2,
		.flags        = UF2_FLAG_FILE,
		.file_offset  = 0, // per-block
		.data_bytes   = 0, // per-block
		.block_number = 0, // per-block
		.block_count  = 0, // global, filled in second pass
		.file_size    = file_bytes, // per-file
		.magic3       = UF2_MAGIC_3
	};

	int name_chars = strlen(filename);
	if (name_chars > UF2_FILENAME_MAX)
		name_chars = UF2_FILENAME_MAX;

	long max_payload = sizeof(block.data) - name_chars - 1;
	if (max_payload > UF2_PAYLOAD_MAX)
		max_payload = UF2_PAYLOAD_MAX;

	int blks = 0;
	long done = 0;

	fprintf(stderr, "file '%s' length %ld\n", filename, file_bytes);
	while (done < file_bytes)
	{
		int remain = file_bytes - done;
		int req = remain <= max_payload ? remain : max_payload;

		int data_len = fread(block.data, 1, req, src_fp);
		if (data_len != req)
			return -ERR_IO;

		strncpy((char *) &block.data[data_len], filename, name_chars);
		memset(&block.data[data_len + name_chars], '\0', sizeof(block.data) - data_len - name_chars);

		block.file_offset  = done;
		block.data_bytes   = req;
		block.block_number = start_block + blks;
		if (fwrite(&block, sizeof(block), 1, dst_fp) != 1)
			return -ERR_IO;

		blks += 1;
		done += req;
	}
	return blks;
}

/**
 * @brief Fix block count field in UF2 block headers
 * @param [in] dst_fp Destination file
 * @param [in] start Start offset
 * @param [in] blocks Total block count
 * @retval block count on success, -error on failure.
 */
int uf2_fixup_block_count(FILE *dst_fp, long start, int blocks)
{
	uint32_t blks = blocks;

	for (uint32_t blk = 0; blk < blks; blk++)
	{
		if (fseek(dst_fp, start + 512 * blk + 6 * sizeof(uint32_t), SEEK_SET) != 0)
			return -ERR_IO;
		if (fwrite(&blks, sizeof(uint32_t), 1, dst_fp) != 1)
			return -ERR_IO;
	}

	return blocks;
}
