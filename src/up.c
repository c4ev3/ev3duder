#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev3_io.h"

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
#include "funcs.h"

#define CHUNK_SIZE 1000 // EV3's HID driver doesn't do packets > 1024B

int up(FILE *fp, const char *dst)
{
    int res;

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if ((unsigned long)fsize > (u32)-1)
      return ERR_FTOOBIG;   

    size_t extra_chunks   = fsize / CHUNK_SIZE;
    size_t final_chunk_sz = fsize % CHUNK_SIZE;

    fprintf(stderr, "Attempting file upload (%ldb total; %zu chunks): \n", fsize, extra_chunks + 1);

    CONTINUE_DOWNLOAD **cd = malloc((1 + extra_chunks) * sizeof(*cd));
    {
        size_t ret;
        size_t i = 0;
        for (; i < extra_chunks; ++i)
        {
            cd[i] = packet_alloc(CONTINUE_DOWNLOAD, CHUNK_SIZE);
            ret = fread(cd[i]->fileChunk, 1, CHUNK_SIZE, fp);
        }

        cd[i] = packet_alloc(CONTINUE_DOWNLOAD, final_chunk_sz);

        ret = fread(cd[i]->fileChunk, 1, final_chunk_sz, fp);
        (void) ret;
        fclose(fp);
    }

    //TODO: read in chunks, whatif long isnt big enough
    BEGIN_DOWNLOAD *bd = packet_alloc(BEGIN_DOWNLOAD, strlen(dst) + 1);
    bd->fileSize = fsize;
    strcpy(bd->fileName, dst);

    res = ev3_write(handle, (u8 *)bd, bd->packetLen + PREFIX_SIZE);
    if (res < 0)
    {
        errmsg = "Unable to write BEGIN_DOWNLOAD.";
        hiderr = ev3_error(handle);
        return ERR_HID;
    }
    fputs("Checking reply: \n", stderr);

    BEGIN_DOWNLOAD_REPLY bdrep;

    res = ev3_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
    if (res <= 0)
    {
        errmsg = "Unable to read BEGIN_DOWNLOAD";
        hiderr = ev3_error(handle);
        return ERR_HID;
    }

    if (bdrep.type == VM_ERROR)
    {
        if (bdrep.ret < ARRAY_SIZE(ev3_error_msgs))
            hiderr = ev3_error_msgs[bdrep.ret];
        else
            hiderr = L"ERROR_OUT_OF_BOUNDS";

        errmsg = "BEGIN_DOWNLOAD was denied.";
        return ERR_VM;
    }

    for (size_t i = 0; i <= extra_chunks; ++i)
    {
        cd[i]->fileHandle = bdrep.fileHandle;
        res = ev3_write(handle, (u8 *)cd[i], cd[i]->packetLen + PREFIX_SIZE);

        if (res < 0)
        {
            errmsg = "Unable to write CONTINUE_DOWNLOAD.";
            hiderr = ev3_error(handle);
            return ERR_HID;
        }
        res = ev3_read_timeout(handle, (u8 *)&bdrep, sizeof bdrep, TIMEOUT);
        if (res <= 0)
        {
            errmsg = "Unable to read CONTINUE_DOWNLOAD";
            hiderr = ev3_error(handle);
            return ERR_HID;
        }

    }

    if (bdrep.type == VM_ERROR)
    {
        if (bdrep.ret < ARRAY_SIZE(ev3_error_msgs))
            hiderr = ev3_error_msgs[bdrep.ret];
        else
            hiderr = L"ERROR_OUT_OF_BOUNDS";

        fputs("Transfer failed.\nlast_reply=", stderr);

        print_bytes(&bdrep, bdrep.packetLen);

        errmsg = "CONTINUE_DOWNLOAD was denied.";
        return ERR_VM;
    }

    fprintf(stderr, "Transfer has been successful! (ret=%d)\n", bdrep.type);

    errmsg = "`upload` was successful.";
    return ERR_UNK;
}

