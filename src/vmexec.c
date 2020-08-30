/**
 * @file vmexec.c
 * @author Jakub Vanek
 * @copyright (c) 2020 Jakub Vanek. Code available under terms of the GNU General Public License 3.0
 * @brief executes lms2012 direct commands
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
 * @brief Execute EV3 bytecode file in the direct command VM
 * @param [in] in Input bytecode file
 * @param [in] out Output file for globals, or NULL
 * @param [in] locals Number of bytecode locals
 * @param [in] globals Number of bytecode globals
 * @param [in] use_reply Whether to wait for the VM reply
 * @retval error according to enum #ERR
 */
int vmexec(FILE *in, FILE *out, int locals, int globals, int use_reply)
{
	int max_locals    = (1 << 6) - 1;
	int max_globals   = (1 << 10) - 1;
	int max_bytecodes = 1024 - (sizeof(VM_CMD) - PREFIX_SIZE + 2);

	if (use_reply)
		max_globals = 1024 - sizeof(VM_REPLY); // prevent reply overflow

	if (locals < 0) locals = 0;
	if (globals < 0) globals = 0;

	if (locals > max_locals) {
		fprintf(stderr, "<%d> is too many local variables\n", locals);
		return ERR_ARG;
	}
	if (globals > max_globals) {
		fprintf(stderr, "<%d> is too many global variables\n", globals);
		return ERR_ARG;
	}

	EXECUTE_FILE *request = packet_alloc(EXECUTE_FILE, max_bytecodes);
	int real = fread(request->bytes, 1, max_bytecodes, in);
	if (!feof(in)) {
		fputs("input bytecode is too long\n", stderr);
		return ERR_IO;
	}
	if (real == 0) {
		fputs("input bytecode is too short\n", stderr);
		return ERR_IO;
	}
	request->replyType = use_reply ? 0x00 : 0x80;
	request->alloc = (u16) locals << 10 | (u16) globals;
	request->packetLen = real + sizeof(VM_CMD) - PREFIX_SIZE;

	int res = ev3_write(handle, (u8 *) request, request->packetLen + PREFIX_SIZE);
	if (res < 0)
	{
		errmsg = "Unable to write VM_CMD.";
		return ERR_COMM;
	}

	if (!use_reply) {
		errmsg = "`vmexec` was successful.";
		return ERR_UNK;
	}

	int reply_size = sizeof(VM_REPLY) + globals;
	VM_REPLY *reply = malloc(reply_size);

	res = ev3_read_timeout(handle, (u8 *) reply, reply_size, TIMEOUT);
	if (res <= 0)
	{
		errmsg = "Unable to read VM_REPLY";
		return ERR_COMM;
	}

	if (reply->replyType != VM_DIRECT_OK)
	{
		fputs("Operation failed.\nlast_reply=", stderr);
		print_bytes(reply, reply->packetLen + 2);

		errno = reply->replyType;
		errmsg = "`vmexec` failed.";
		return ERR_VM;
	}

	if (out) {
		fwrite(reply->bytes, 1, globals, out);
	}

	if (!out || out != stdout) {
		errmsg = "`vmexec` was successful.";
	}

	return ERR_UNK;
}
