/**
 * @file run.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 2.0
 * @brief runs rbf file via VM facilities
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"
#include "ev3_io.h"

/** copied 1:1 from the LEGO communication developer manual */
static const u8 run1[] = {0xC0, 0x08, 0x82, 0x01, 0x00, 0x84};
static const u8 run2[] = {0x60, 0x64, 0x03, 0x01, 0x60, 0x64, 0x00};

/**
 * @brief run rbf file
 * @param [in] path path to file to execute
 *
 * @retval error according to enum #ERR
 * @see http://ev3.fantastic.computer/doxygen/buildinapps.html
 * @see mkrbf()
 */
int run(const char *exec)
{
    int res;
    VM_REPLY reply;
    size_t exec_sz = strlen(exec) + 1;

    EXECUTE_FILE *run = packet_alloc(EXECUTE_FILE, sizeof (run1) + exec_sz + sizeof (run2));

// *INDENT-OFF*
	mempcpy(mempcpy(mempcpy((u8 *)&run->bytes, // run->bytes = [run1] + [exec] + [run2]
	                        run1, sizeof run1),
      	                	exec,  exec_sz),
	                  			run2, sizeof run2);
// *INDENT-ON*

//FIXME: inquire whether start succeeded. check communicaton developer manual  pdf (debug mode maybe?)
    res = ev3_write(handle, (u8 *)run, run->packetLen + PREFIX_SIZE);
    if (res < 0)
    {
        hiderr = ev3_error(handle);
        return ERR_HID;
    }

    res = ev3_read_timeout(handle, (u8 *)&reply, sizeof reply + 2, TIMEOUT / 10);
    if (res < 0)
    {
        errmsg = NULL;
        hiderr = ev3_error(handle);
        return ERR_HID;
    }
    else if (res == 0 || (res > 0 && memcmp(&reply, &EXECUTE_FILE_REPLY_SUCCESS, sizeof reply) == 0))
        //FIXME!!!!!!!
    {
        errmsg = "`exec` has been successful.";
        return ERR_UNK;
    }
    else
    {
        //TODO: check for underlying app first?
        errmsg = "`exec` failed.";
        return ERR_VM;
    }
}

