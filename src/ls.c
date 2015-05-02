#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
#include "funcs.h"

extern hid_device *handle;

int ls(const char *path)
{
    int res;
    size_t path_sz = strlen(path) + 1;
    LIST_FILES *list = packet_alloc(LIST_FILES, path_sz);
    list->maxBytes = 0xffff;
    memcpy(list->path, path, path_sz);

    print_bytes(list, list->packetLen + PREFIX_SIZE);
    res = hid_write(handle, (u8 *)list, list->packetLen + PREFIX_SIZE);
    if (res < 0)
    {
        errmsg = "Unable to write BEGIN_DOWNLOAD.";
        hiderr = hid_error(handle);
        return ERR_HID;
    }
    fputs("Checking reply: \n", stderr);
    size_t listrep_sz = sizeof(LIST_FILES_REPLY) + list->maxBytes;
    LIST_FILES_REPLY *listrep = malloc(listrep_sz);

    res = hid_read_timeout(handle, (u8 *)listrep, listrep_sz, TIMEOUT);
    if (res <= 0)
    {
        errmsg = "Unable to read LIST_FILEShiderr";
        hiderr = hid_error(handle);
        return ERR_HID;
    }

    if (listrep->type == VM_ERROR)
    {
        if (listrep->ret < ARRAY_SIZE(ev3_error))
            hiderr = ev3_error[listrep->ret];
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

