/**
 * @file uf2_helper.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief misc lowlevel functions for UF2 code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ev3_io.h"

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "uf2.h"

#ifdef __WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

/**
 * @brief Get last path component
 * @param [in] fullname Full file path
 * @param [in] backslash_trigger Count backslash as file separator
 * @retval fullname pointer advanced so that it points after the last backslash (if any).
 */
const char *uf2_basename(const char *fullname, int backslash_trigger)
{
	int len = strlen(fullname);
	int pos = len;
	while (fullname[pos] != '/' && (!backslash_trigger || fullname[pos] != '\\') && pos >= 0)
		pos--;
	return &fullname[pos + 1];
}

#ifdef __WIN32

static int uf2_mkdir_impl(char *path, char *start) {
	char *nextSlash = start;

	while (nextSlash[0] == '/' || nextSlash[0] == '\\') nextSlash++;
	while (nextSlash[0] != '/' && nextSlash[0] != '\\' && nextSlash[0] != '\0') nextSlash++;

	// do not create directory for the final part (destination file)
	if (nextSlash[0] == '\0') return 0;

	// skip C:
	if (!( (nextSlash - path) == 2 && path[1] == ':' ))
	{
		nextSlash[0] = '\0';
		if (!CreateDirectoryA(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return -1;
		nextSlash[0] = '/';
	}
	return uf2_mkdir_impl(path, nextSlash + 1);
}

#else

static int uf2_mkdir_impl(char *path, char *start) {
	char *nextSlash = start;

	while (nextSlash[0] == '/') nextSlash++;
	while (nextSlash[0] != '/' && nextSlash[0] != '\0') nextSlash++;

	// do not create directory for the final part (destination file)
	if (nextSlash[0] == '\0') return 0;

	nextSlash[0] = '\0';
	if (mkdir(path, 00755) < 0 && errno != EEXIST) return -1;
	nextSlash[0] = '/';
	return uf2_mkdir_impl(path, nextSlash + 1);
}

#endif

int uf2_mkdir_for(const char *filename)
{
	char *mutable = strdup(filename);
	int result = uf2_mkdir_impl(mutable, mutable);
	free(mutable);
	return result;
}

#ifdef __WIN32

int uf2_write_into(const char *realname, uint8_t *data,
				   uint32_t data_bytes, uint32_t file_offset,
				   uint32_t file_size)
{
	HANDLE hnd = INVALID_HANDLE_VALUE;
	hnd = CreateFileA(realname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
	                  FILE_ATTRIBUTE_NORMAL, NULL);
	if (hnd == INVALID_HANDLE_VALUE) goto fail;

	if (SetFilePointer(hnd, file_size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) goto fail;
	if (!SetEndOfFile(hnd)) goto fail;
	if (SetFilePointer(hnd, file_offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) goto fail;

	DWORD real = 0;
	if (!WriteFile(hnd, data, data_bytes, &real, NULL)) goto fail;
	if (real != data_bytes) goto fail;

	CloseHandle(hnd);
	return 0;
fail:
	fprintf(stderr, "Windows error: 0x%08X\n", GetLastError());
	if (hnd != INVALID_HANDLE_VALUE)
		CloseHandle(hnd);
	return -1;
}

#else

int uf2_write_into(const char *realname, uint8_t *data,
				   uint32_t data_bytes, uint32_t file_offset,
				   uint32_t file_size)
{
	int error = 0;
	int fd = open(realname, O_WRONLY | O_CREAT, 0644);
	if (fd < 0) return -1;

	if (ftruncate(fd, file_size) < 0)         goto fail;
	if (lseek(fd, file_offset, SEEK_SET) < 0) goto fail;
	if (write(fd, data, data_bytes) < 0)      goto fail;

	close(fd);
	return 0;
fail:
	error = errno;
	close(fd);
	errno = error;
	return -1;
}

#endif
