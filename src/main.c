#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <hidapi.h>

#include "defs.h"
#include "systemcmd.h"
#include "error.h"

#define VendorID 0x694 /* LEGO GROUP */
#define ProductID 0x005 /* EV3 */
#define SerialID NULL

extern struct error ev3_up(FILE*, const char*);
extern struct error ev3_dl(const char*, FILE*);
extern struct error ev3_test(void);
extern struct error ev3_exec(const char*);
extern struct error ev3_ls(const char*);
extern struct error ev3_raw(const char*, size_t len);

//TODO: use these instead of struct error:
const char *errstr;

void params_print()
{
    puts(	"USAGE: ev3duder [ up loc rem | dl rem loc | exec rem | kill rem |"	"\n"
            "                 cp rem rem | mv rem rem | rm rem | ls [rem] | tree [rem] |"	"\n"
            "                 shell | raw space_seperated_hex_bytes | test ]"					"\n"
            "\n"
            "       rem = remote (EV3) system path, loc = (local file system) path"	"\n");
}
enum ARGS             {ARG_TEST, ARG_UP, ARG_EXEC, ARG_LS, ARG_RAW, ARG_END};
const char *args[] =  {"test",   "up",   "exec", "ls", "raw", NULL};

hid_device *handle;

int main(int argc, char *argv[])
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
    for (i = 0; i < ARG_END; ++i)
        if (strcmp(argv[1], args[i]) == 0) break;
    argc -= 2;
    argv += 2;
    struct error ret;
    switch (i)
    {
    case ARG_TEST:
        assert(argc == 0);
        ret = ev3_test();
        break;

    case ARG_UP:
        assert(argc == 2);
        {
            FILE *fp = fopen(argv[0], "rb");
            if (!fp)
            {
                printf("File <%s> doesn't exist.\n", argv[0]);
                return ERR_IO;
            }
            ret = ev3_up(fp, argv[1]);
        }
        break;

    case ARG_EXEC:
        assert(argc == 1);
        ret = ev3_exec(argv[0]);
        break;

    case ARG_END:
        params_print();
        return ERR_ARG;

    case ARG_LS:
        assert(argc <= 1);
        {
        const char* arg = argv[0] ?: ".";
        ret = ev3_ls(arg);
        }
        break;
    case ARG_RAW:
        assert(argc > 0);
        if(0){
          size_t lens[argc];
          size_t lens_total = 0;
          for (int i = 0; i < argc; i++)
            lens_total += (lens[i] = strlen(argv[i]));
          char *cmd = malloc(lens_total + 1);
          char *ptr = cmd;
          for (int i = 0; i < argc; i++)
              ptr = mempcpy(ptr, argv[i], lens[i]);        
          *ptr = '\0'; 
          ret = ev3_raw(cmd, lens_total);
        }
        else
         {
            u8 cmd[] = "\x00\x0a\x3f\x00\x00\x01\x99";
            cmd[1] = sizeof cmd; //only bis ff
            hid_write(handle, cmd, sizeof cmd - 1); 
         }
        break;
    default:
        printf("<%s> hasn't been implemented yet.", argv[0]);
    }

    if (ret.category == ERR_HID)
        fprintf(stdout, "%ls\n", (wchar_t*)ret.reply);
    else
        fprintf(stdout, "%s (%s)\n", ret.msg, (char*)ret.reply ?: "null");

    // maybe \n to stderr?
    return ret.category;
}

