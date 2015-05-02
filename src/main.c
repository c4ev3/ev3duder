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
            "shell str | pwd [rem] | test ]\n"
            "\n"
            "       "
            "rem = remote (EV3) system path, loc = (local file system) path"	"\n");
}
#define FOREACH_ARG(ARG) \
    ARG(test)            \
    ARG(up)              \
    ARG(dl)              \
    ARG(exec)            \
    ARG(pwd)             \
    ARG(ls)              \
    ARG(rm)              \
    ARG(mkdir)           \
    ARG(end)

#define MK_ENUM(x) ARG_##x,
#define MK_STR(x) T(#x),
enum ARGS { FOREACH_ARG(MK_ENUM) };
const tchar *args[] = { FOREACH_ARG(MK_STR) };

hid_device *handle;
tchar * tstrjoin(tchar *, tchar *, size_t *);
int main(int argc, tchar *argv[])
{
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
    tchar *cd = tgetenv(T("CD"));
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
            size_t len;
            tchar *path = tstrjoin(cd, argv[1], &len); 
            if (!fp)
            {
                printf("File <%" PRIts "> doesn't exist.\n", argv[0]);
                return ERR_IO;
            }
            ret = up(fp, U8(path, len));
        }
        break;

    case ARG_exec:
        assert(argc == 1);
        {
        size_t len;
        tchar *path = tstrjoin(cd, argv[0], &len); 
        ret = exec(U8(path, len));
        }
        break;

    case ARG_end:
        params_print();
        return ERR_ARG;
    case ARG_pwd:
        assert(argc <= 1);
        {
        size_t len;
        tchar *path = tstrjoin(cd, argv[0], &len); 
        printf("path:%" PRIts " (len=%zu)\nUse export CD=dir to cd to dir\n",
            path ?: T("."), len);  
        }
        return ERR_UNK;
    case ARG_ls:
        assert(argc <= 1);
        {
        size_t len;
        tchar *path = tstrjoin(cd, argv[0], &len); 
        ret = ls(len ? U8(path, len) : ".");
        }
        break;

    case ARG_mkdir:
        assert(argc == 1);
        {
        size_t len;
        tchar *path = tstrjoin(cd, argv[0], &len); 
        ret = mkdir(U8(path, len));
        }
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
            size_t len;
            tchar *path = tstrjoin(cd, argv[0], &len); 
            ret = rm(U8(path, len));
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
tchar *tstrjoin(tchar *s1, tchar *s2, size_t *len)
{
    if (s1 == NULL || *s1 == '\0') {
      if (s2 != NULL)
        *len = tstrlen(s2);
      else
        *len = 0;
      return s2;
    }
    if (s2 == NULL || *s2 == '\0') {
      if (s1 != NULL)
        *len = tstrlen(s1);
      else
        *len = 0;
      return s1;
    }

    size_t s1_sz = tstrlen(s1),
           s2_sz = tstrlen(s2);
    *len = sizeof(tchar[s1_sz+s2_sz]);
    tchar *ret = malloc(*len+1);
    mempcpy(mempcpy(ret,
                s1, sizeof(tchar[s1_sz+1])),
                s2, sizeof(tchar[s2_sz+1])
           );
    ret[s1_sz] = '/';
    return ret;
}

