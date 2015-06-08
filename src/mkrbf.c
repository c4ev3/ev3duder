#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "defs.h"
#include "error.h"
#include "funcs.h"

//calling with buf = null is undefined behaviour
static const char magic[] = "LEGO";
static const char before[] = 
"\x68\x00\x01\x00\x00\x00\x00\x00\x1C\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x60\x80";
static const char after[] = "\x44\x85\x82\xE8\x03\x40\x86\x40\x0A";
size_t mkrbf(char **buf, const char *cmd)
{
		size_t cmd_sz = strlen(cmd);
        u32 static_sz = (u32)(sizeof magic -1 + 4 + sizeof before -1 + cmd_sz + 1 + sizeof after -1);
		*buf = malloc(static_sz);
		(void)mempcpy(mempcpy(mempcpy(mempcpy(mempcpy(*buf,
					   	magic, sizeof magic -1),
						&static_sz, 4),
						before, sizeof before - 1),
						cmd, cmd_sz + 1),
						after, sizeof after -1);
		return static_sz;
}
