/**
 * @file uf2_unpack.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief extract files from microsoft uf2 container
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

static void uf2_map(char *dst, size_t dstlen, const char *dstdir, const char *filename, int use_paths);

/**
 * @brief Unpack files from a UF2 file
 * UF2 specification is available at https://github.com/microsoft/uf2/blob/master/README.md
 * @param [in] fp UF2 input stream
 * @param [in] dstdir Where to extract the files
 * @param [in] use_paths Whether to preserve paths from the UF2 file
 * @retval error according to enum #ERR
 * @bug might not handle files bigger than 2gb :-)
 */
int uf2_unpack(FILE *fp, const char *dstdir, int use_paths)
{
	char realname[256];
	uf2_block_t *block = malloc(sizeof(uf2_block_t));
	if (!block) return ERR_NOMEM;

	while (1)
	{
		if (fread(block, 512, 1, fp) < 1)
			break;

		if (block->magic1 != UF2_MAGIC_1 ||
		    block->magic2 != UF2_MAGIC_2 ||
		    block->magic3 != UF2_MAGIC_3)
		    continue;

		if ((block->flags & UF2_FLAG_IGNORE) != 0 ||
		    (block->flags & UF2_FLAG_FILE)   == 0)
			continue;

		if (block->data_bytes > UF2_PAYLOAD_MAX ||
		    block->file_size > UF2_FILE_MAX ||
		    block->file_offset >= block->file_size)
			continue;

		block->data[475] = 0;
		const char *name = (const char *) &block->data[block->data_bytes];
		int name_chars = strlen(name);
		if (name_chars > UF2_FILENAME_MAX)
			continue;

		uf2_map(realname, sizeof(realname), dstdir, name, use_paths);

		int ret = uf2_mkdir_for(realname);
		if (ret < 0) {
			perror("cannot make directory for destination uf2 file");
			errmsg = "`uf2 unpack` has failed.";
			return -ret;
		}

		ret = uf2_write_into(realname, block->data, block->data_bytes, block->file_offset, block->file_size);
		if (ret < 0) {
			perror("cannot write uf2 file contents");
			errmsg = "`uf2 unpack` has failed.";
			return -ret;
		}
	}
	errmsg = "`uf2 unpack` was successful.";
	return ERR_UNK;
}

void uf2_map(char *dst, size_t dstlen, const char *dstdir, const char *filename, int use_paths)
{
	const char *basename = uf2_basename(filename, 0);
	char *foldername;
	if (use_paths) {
		foldername = strdup(filename);
		char *slash = strchr(foldername, '/');
		if (slash != NULL)
			*slash = '\0';
		if (strcmp(foldername, "..") == 0)
			strcpy(foldername, ".");
	} else {
		foldername = strdup(".");
	}

#ifdef __WIN32
	_snprintf(dst, dstlen, "%s\\%s\\%s", dstdir, foldername, basename);
	dst[dstlen - 1] = '\0';
#else
	snprintf(dst, dstlen, "%s/%s/%s", dstdir, foldername, basename);
#endif
	free(foldername);
}
