#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
extern hid_device *handle;
static const u8 ls[] =
    "\xa3\xf0\x00\x00\x01\x99\xff\xff";
static const u8 run1[] = {0xC0, 0x08, 0x82, 0x01, 0x00, 0x84};
static const u8 run2[] = {0x60, 0x64, 0x03, 0x01, 0x60, 0x64, 0x00};

struct error ev3_ls(const char *path)
{
    int res;
    size_t path_sz = strlen(path) + 1;
    LIST_FILES *list = packet_alloc(LIST_FILES, path_sz);
    list->maxBytes = 0xff;
    memcpy(list->path, path, path_sz);

//FIXME: inquire whether start succeeded. check communicaton developer manual  pdf (debug mode maybe?)
    print_bytes(list, list->packetLen + PREFIX_SIZE);
    res = hid_write(handle, (u8 *)list, list->packetLen + PREFIX_SIZE);
    if (res < 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to write BEGIN_DOWNLOAD.", .reply = hid_error(handle)
    };
    fputs("Checking reply: \n", stderr);
    size_t listrep_sz = sizeof(LIST_FILES_REPLY) + list->maxBytes;
    LIST_FILES_REPLY *listrep = malloc(listrep_sz);

    res = hid_read_timeout(handle, (u8 *)listrep, listrep_sz, TIMEOUT);
    if (res <= 0) return (struct error)
    {
        .category = ERR_HID, .msg = "Unable to read LIST_FILES_REPLY", .reply = hid_error(handle)
    };

    if (listrep->type == VM_ERROR)
    {
        const char*msg = "ERROR_OUT_OF_BOUNDS";
        print_bytes(listrep, sizeof *listrep);
        return (struct error)
        {
            .category = ERR_VM, .msg = "LIST_FILES was denied.", .reply = msg
        };
    }


    fputs("=LIST_FILES_REPLY\n", stderr);
    print_bytes(listrep, sizeof *listrep);
    char *end = strchr(listrep->list, '\n');
    printf("<%d bytes>", (int)(end-listrep->list));
    printf("\nFilelist=%.*s\n", (int)(end-listrep->list), listrep->list);
    print_bytes(listrep->list, 200);
    putc('\n', stderr);
    fputs("=cut\n", stderr);

    return (struct error)
    {
        .category = ERR_UNK, .msg = "`LIST_FILES` was successful?", .reply = NULL
    };

}

