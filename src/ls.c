#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev3_io.h"

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
#include "funcs.h"

/**
 * @brief List files and directories at \p path. path is always relative to <em>/home/root/lms2012/prjs/sys/<em>
 * @param [in] loc Local FILE*s
 * @param [in] rem Remote EV3 UTF-8 encoded path strings
 *
 * @retval error according to enum #ERR
 * @see http://topikachu.github.io/python-ev3/UIdesign.html
 * @bug Doesn't handle replies over 1000 byte in length.
 *      implementation of \p CONTINUTE_LIST_FILES would be required
 */
int ls(const char *path)
{
    int res;
    size_t path_sz = strlen(path) + 1;
    LIST_FILES *list = packet_alloc(LIST_FILES, path_sz);
    list->maxBytes = 0xffff;
    memcpy(list->path, path, path_sz);

    print_bytes(list, list->packetLen + PREFIX_SIZE);
    res = ev3_write(handle, (u8 *)list, list->packetLen + PREFIX_SIZE);
    if (res < 0)
    {
        errmsg = "Unable to write LIST_FILES.";
        hiderr = ev3_error(handle);
        return ERR_HID;
    }
    fputs("Checking reply: \n", stderr);
    size_t listrep_sz = sizeof(LIST_FILES_REPLY) + list->maxBytes;
    LIST_FILES_REPLY *listrep = malloc(listrep_sz);

    res = ev3_read_timeout(handle, (u8 *)listrep, listrep_sz, TIMEOUT);
    if (res <= 0)
    {
        errmsg = "Unable to read LIST_FILES";
        hiderr = ev3_error(handle);
        return ERR_HID;
    }

    if (listrep->type == VM_ERROR)
    {
        if (listrep->ret < ARRAY_SIZE(ev3_error_msgs))
            hiderr = ev3_error_msgs[listrep->ret];
        else
            hiderr = L"ERROR_OUT_OF_BOUNDS";
        fputs("Operation failed.\nlast_reply=", stderr);
        print_bytes(listrep, listrep->packetLen);


        errmsg = "`LIST_FILES` was denied.";
        return ERR_VM;
    }
    puts(listrep->list);
    //README: it's assumed that the folder structure fits into ONE HID packet. if not additional code for handling CONTINUTE_LIST_FILES is required

    errmsg = "`LIST_FILES` was successful.";
    return ERR_UNK;

}

