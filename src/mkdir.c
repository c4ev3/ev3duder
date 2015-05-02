#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
#include "funcs.h"

extern hid_device *handle;

int mkdir(const char *path)
{
    int res;
    size_t path_sz = strlen(path) + 1;
    CREATE_DIR *mk = packet_alloc(CREATE_DIR, path_sz);
    memcpy(mk->path, path, path_sz);

    res = hid_write(handle, (u8 *)mk, mk->packetLen + PREFIX_SIZE);
    if (res < 0) {
        errmsg = "Unable to write CREATE_DIR.";
        hiderr = hid_error(handle);
        return ERR_HID;
    }
    fputs("Checking reply: \n", stderr);
    CREATE_DIR_REPLY mkrep;

    res = hid_read_timeout(handle, (u8 *)&mkrep, sizeof mkrep, TIMEOUT);
    if (res <= 0)
    {
        errmsg = "Unable to read CREATE_DIR_REPLY";
        hiderr = hid_error(handle);
        return ERR_HID;
    }

    if (mkrep.type == VM_ERROR)
    {
        if (mkrep.ret < ARRAY_SIZE(ev3_error))
            hiderr = ev3_error[mkrep.ret];
        else
            hiderr = L"ERROR_OUT_OF_BOUNDS";
        fputs("Operation failed.\nlast_reply=", stderr);
        print_bytes(&mkrep, mkrep.packetLen);


        errmsg = "`CREATE_DIR` was denied.";
        return ERR_VM;
    }

    errmsg = "`CREATE_DIR` was successful.";
    return ERR_UNK;
}

