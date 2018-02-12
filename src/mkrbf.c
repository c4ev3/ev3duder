/**
 * @file mkrbf.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief call ev3 shell via rbf file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "defs.h"
#include "funcs.h"

//calling with buf = null is undefined behaviour
static const char magic[] = "LEGO"; // MAGIC at start of RBF files
static const char before[] =
		"\x68\x00\x01\x00\x00\x00\x00\x00\x1C\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x60\x80";
static const char after[] = "\x44\x85\x82\xE8\x03\x40\x86\x40\x0A";

/**
 * @brief Offline functionality. Generates bytecode for executing a binary executable
 * @param [out] buf buffer with the bytecode
 * @param [in] cmd the command to be executed
 *
 * @retval bytes_written buffer size
 * @warning Command is passed unsanitized to root shell. Handle with care
 */
size_t mkrbf(char **buf, const char *cmd)
{
	size_t cmd_sz = strlen(cmd);
	u32 static_sz = (u32) (sizeof magic - 1 + 4 + sizeof before - 1 + cmd_sz + 1 + sizeof after - 1);
	*buf = malloc(static_sz);
	(void) mempcpy(mempcpy(mempcpy(mempcpy(mempcpy(*buf,
												   magic, sizeof magic - 1),
										   &static_sz, 4),
								   before, sizeof before - 1),
						   cmd, cmd_sz + 1),
				   after, sizeof after - 1);
	return static_sz;
}
// funny trivia: This is technically undefined behavior (Translation unit not ending with line break)
