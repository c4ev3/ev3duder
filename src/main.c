#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <hidapi.h>
#include <wchar.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"
#include "utf8.h"
#include "funcs.h"

#define VendorID 0x694 /* LEGO GROUP */
#define ProductID 0x005 /* EV3 */
#define SerialID NULL

const char *errmsg;
const wchar_t *hiderr;

void params_print()
{
    puts(	"USAGE: ev3duder "
            "[ up loc rem | dl rem loc | exec rem | kill rem |\n"
            "                  "
            "cp rem rem | mv rem rem | rm rem | ls [rem] | tree [rem] |\n"
            "                  "
            "shell str | cd rem | pwd | test ]\n"
            "\n"
            "       "
            "rem = remote (EV3) system path, loc = (local file system) path"	"\n");
}
#define FOREACH_ARG(ARG) \
    ARG(test)            \
    ARG(up)              \
    ARG(dl)              \
    ARG(exec)            \
    ARG(ls)              \
    ARG(rm)              \
    ARG(mkdir)           \
    ARG(end)

#define MK_ENUM(x) ARG_##x,
#define MK_STR(x) T(#x),
enum ARGS { FOREACH_ARG(MK_ENUM) };
const tchar *args[] = { FOREACH_ARG(MK_STR) };

hid_device *handle;
int main(int _argc, tchar *_argv[])
{
    int argc = _argc;
    tchar **argv = _argv;

    if (argc == 1)
    {
        params_print();
        return ERR_ARG;
    }
    handle = hid_open(VendorID, ProductID, SerialID);
    if (!handle)
    {
        puts("EV3 not found. Is it properly plugged into the USB port?");
        return ERR_HID;
    }

    int i;
    for (i = 0; i < ARG_end; ++i)
        if (tstrcmp(argv[1], args[i]) == 0) break;

    argc -= 2;
    argv += 2;
    int ret;

    switch (i)
    {
    case ARG_test:
        assert(argc == 0);
        ret = test();
        break;

    case ARG_up:
        assert(argc == 2);
        {
            FILE *fp = tfopen(argv[0], "rb");
            if (!fp)
            {
                printf("File <%" PRIts "> doesn't exist.\n", argv[0]);
                return ERR_IO;
            }
            ret = up(fp, U8(argv[1]));
        }
        break;

    case ARG_exec:
        assert(argc == 1);
        ret = exec(U8(argv[0]));
        break;

    case ARG_end:
        params_print();
        return ERR_ARG;

    case ARG_ls:
        assert(argc <= 1);
        ret = ls(U8(argv[0]) ?: ".");
        break;

    case ARG_mkdir:
        assert(argc == 1);
        ret = mkdir(U8(argv[0]));
        break;
    case ARG_rm:
        assert(argc == 1);
        {
            if(tstrchr(argv[0], '*'))
            {
                printf("This will probably remove more than one file. Are you sure to proceed? (Y/n)");
                char ch;
                scanf("%c", &ch);
                if ((ch | ' ') == 'n') // check if 'n' or 'N'
                {
                    ret = ERR_UNK;
                    break;
                }
                fflush(stdin); // technically UB, but POSIX and Windows allow for it
            }
            ret = rm(U8(argv[0]));
        }
        break;
    default:
        ret = ERR_ARG;
        printf("<%" PRIts "> hasn't been implemented yet.", argv[0]);
    }

    if (ret == ERR_HID)
        fprintf(stdout, "%ls\n", (wchar_t*)hiderr);
    else
        fprintf(stdout, "%s (%s)\n", errmsg, (char*)hiderr ?: "null");

    // maybe \n to stderr?
    return ret;
}

