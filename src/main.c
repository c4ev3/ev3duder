/**
 * @file main.c
 * @author Ahmad Fatoum
 * @brief Argument parsing, device opening and file creation/openning
 */
//! For avoiding the need to separately define and declare stuff
#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <errno.h>

#undef assert
//FIXME: add better error message
#define assert(cond) do{ if (!(cond)) {if (handle) ev3_close(handle);exit(ERR_ARG);}}while(0)

#include <hidapi.h>
#include "btserial.h"
#include "tcp.h"
#include "ev3_io.h"

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

//! LEGO GROUP
#define VendorID 0x694
//! EV3
#define ProductID 0x005 /* EV3 */
//! Filename used for executing command with `exec`
#define ExecName  "/tmp/Executing shell cmd.rbf"
// FIXME: general solution
//! Filename used for saving output with `exece` (tmp won't work because it's not readable? wtf)
#define OutputName "/dev/lms_usbdev"

const char* const params =
    "USAGE: ev3duder " "[ --tcp | --usb | --serial | --stdio ] [-d dev | -d2 dev1 dev2]\n"
    "                " "[ up loc rem | dl rem loc | rm rem | ls [rem] |\n"
    "                " "  mkdir rem | mkrbf rem loc | run rem | exec cmd |\n"
    "                " "  wpa2 SSID [pass] | info | tunnel ]\n"
    "       "
    "rem = remote (EV3) path, loc = local path, dev = device identifier"	"\n";
const char* const params_desc =
    " up\t"		"upload local file to remote ev3 brick\n"
    " dl\t"		"download remote file to local system\n"
    " rm\t"		"remove file on ev3 brick\n"
    " ls\t"		"list files. Standard value is '/'\n"
    " info\t"	"attempt a beep and print information about the connection\n"
    " mkdir\t"	"create directory. Relative to path of VM.\n"
    " mkrbf\t"	"create rbf (menu entry) file locally. Sensible upload paths are:\n"
    "\t"		"\t../prjs/BrkProg_SAVE/ Internal memory\n"
    "\t"		"\t../prjs/BrkProg_DL/ Internal memory\n"
    "\t"		"\t./apps/ Internal memory - Applicatios (3rd tab)\n"
    "\t"		"\t/media/card/myapps/ Memory card\n"
    "\t"		"\t/media/usb/myappps/ USB stick\n"
    "run\t"		"instruct the VM to run a rbf file\n"
    "exec\t"	"pass cmd to root shell. Handle with caution\n"
    "wpa2\t"	"connect to WPA-Network SSID, if pass isn't specified, read from stdin\n"
    "tunnel\t"	"connects stdout/stdin to the ev3 VM\n"
    ;

#define FOREACH_ARG(ARG) \
ARG(info)            \
ARG(up)              \
ARG(dl)              \
ARG(run)             \
ARG(ls)              \
ARG(rm)              \
ARG(mkdir)           \
ARG(mkrbf)           \
ARG(tunnel)			\
ARG(listen)			\
ARG(send)			\
ARG(exec)            \
ARG(wpa2)            \
ARG(end)

#define MK_ENUM(x) ARG_##x,
#define MK_STR(x) #x,
enum ARGS { FOREACH_ARG(MK_ENUM) };
static const char *args[] = { FOREACH_ARG(MK_STR) };
static const char *offline_args[] = { MK_STR(mkrbf) /*MK_STR(tunnel)*/ };

static char* chrsub(char *s, char old, char new);
#ifdef _WIN32
#define SANITIZE(s) (chrsub((s), '/', '\\'))
#else
#define SANITIZE
#endif
static struct {
	unsigned select:1;
	unsigned hid:1;
	unsigned serial:1;
	unsigned tcp:1;
} switches;

int main(int argc, char *argv[])
{
    handle = NULL;
    if (argc == 2 && (
                strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "--help") == 0 ||
                strcmp(argv[1], "/?") == 0))
    {
        printf("%s (%s; %s) v%s\n"
               "Copyright (C) 2015 Ahmad Fatoum\n"
               "This is free software; see the source for copying conditions.   There is NO\n"
               "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
               "Source is available under the GNU GPL v3.0 https://github.com/a3f/ev3duder/\n\n",
               argv[0], CONFIGURATION, SYSTEM, VERSION);
        puts(params);
        puts(params_desc);
        return ERR_UNK;
    }
    const char *device = NULL; const char *device2 = NULL;
	(void) device2; // surpress warnings as we don't always use device2
	while (argv[1] && *argv[1] == '-')
	{
		if (argv[1][1] == '-')
		{/* switches */
			char *a = argv[1] + 2;
			if (a == '\0')
			{
				argc--, argv++; break;
			}
			
			if (strcmp("usb", a) == 0 || strcmp("hid", a) == 0)
				switches.select = switches.hid = 1;
			else if (strcmp("tcp", a) == 0 || strcmp("inet", a) == 0)
				switches.select = switches.tcp = 1;
			else if (strcmp("serial", a) == 0 || strcmp("bt", a) == 0)
				switches.select = switches.serial = 1;
			else
			{
				fprintf(stderr, "Invalid switch '%s'\n", argv[1]);
				return ERR_ARG;
			}

			argc--, argv++;
		}else
		{
    		if (strcmp(argv[1]+1, "d") == 0)
    		{
				device = argv[2];
				argv+=2;
				argc-=2;
			} else if (strcmp(argv[1]+1, "d2") == 0)
			{
				device = argv[2];
				device2 = argv[3];
				argv+=3;
				argc-=3;
			} else {
				fprintf(stderr, "Invalid parameter '%s'\n", argv[1]);
				return ERR_ARG;
			}
		}

	}
	if (argc == 1) {
		puts(params);
		return ERR_ARG;
	}

    int i;
    for (i = 0; i < (int)ARRAY_SIZE(offline_args); ++i)
        if (strcmp(argv[1], offline_args[i]) == 0) break;

    if (i == ARRAY_SIZE(offline_args))
    {
        if ((switches.hid || !switches.select) &&
			(handle = hid_open(VendorID, ProductID, NULL))) // TODO: last one is SerialID, make it specifiable via commandline
        {
        if (!switches.select)
            fputs("USB connection established.\n", stderr);
            // the things you do for type safety...
            ev3_write = (int (*)(void*, const u8*, size_t))hid_write;
            ev3_read_timeout = (int (*)(void*, u8*, size_t, int))hid_read_timeout;
            ev3_error = (const wchar_t* (*)(void*))hid_error;
            ev3_close = (void (*)(void*))hid_close;
        }
        else if ((switches.serial || !switches.select) &&
        		 (handle = bt_open(device)))
        {
        if (!switches.select)
            fprintf(stderr, "Bluetooth serial connection established (%s).\n", device);
            ev3_write = bt_write;
            ev3_read_timeout = bt_read;
            ev3_error = bt_error;
            ev3_close = bt_close;
        }
        else if ((switches.tcp || !switches.select) &&
				 (handle = tcp_open(device)))
		{
        if (!switches.select)
		{
			struct tcp_handle *info = (struct tcp_handle*)handle;
            fprintf(stderr, "TCP connection established (%s@%s:%u).\n", info->name, info->ip, info->tcp_port);
		}
			ev3_write = tcp_write;
			ev3_read_timeout = tcp_read;
			ev3_error = tcp_error;
			ev3_close = tcp_close;
			
		}	
		else {
            puts("EV3 not found. Either plug it into the USB port or pair over Bluetooth.\n");
#ifdef __linux__
            puts("Insufficient access to the usb device might be a reason too, try sudo.");
#endif
            return ERR_COMM;
        }
    }


    for (i = 0; i < ARG_end; ++i)
        if (strcmp(argv[1], args[i]) == 0) break;

    argc -= 2;
    argv += 2;

    int ret;
    switch (i)
    {
        FILE *fp = NULL;
        char *buf = NULL;
        size_t len = 0;
    case ARG_info:
        ret = info(argv[0]);
        break;

    case ARG_up:
        assert(argc == 2);
        fp = fopen(SANITIZE(argv[0]), "rb");
        if (!fp)
        {
            printf("File <%s> doesn't exist.\n", argv[0]);
            return ERR_IO;
        }
        ret = up(fp, argv[1]);
        break;
    case ARG_dl:
        assert(argc <= 2);
        if (argc == 1)
        {
            buf = strrchr(argv[0], '/');
            if (buf)
                buf++; // character after slash
            else
                buf = argv[0];
        } else buf = argv[1];

        fp = fopen(buf, "wb"); //TODO: no sanitize here?
        if(!fp)
        {
            printf("File <%s> couldn't be opened for writing.\n", buf);
            return ERR_IO;
        }

        ret = dl(argv[0], fp);
        break;
    case ARG_run:
        assert(argc == 1);
        ret = run(argv[0]);
        break;

    case ARG_end:
        puts(params);
        return ERR_ARG;
    case ARG_ls:
        assert(argc <= 1);
        ret = ls(argv[0] ?: "/");
        break;
	case ARG_tunnel:
        ret = tunnel();
        break;
	case ARG_listen:
        ret = listen();
        break;
	case ARG_send:;
		int send(void);
        ret = send();
        break;
    case ARG_mkdir:
        assert(argc == 1);
        ret = mkdir(argv[0]);
        break;
    case ARG_mkrbf:
        assert(argc >= 2);
        fp = fopen(SANITIZE(argv[1]), "wb");
        if (!fp)
            return ERR_IO;

        len = mkrbf(&buf, argv[0]);
        fwrite(buf, len, 1, fp);
        fclose(fp);
        return ERR_UNK;
    case ARG_wpa2:
        assert(argc >= 1);
        /*{
        #define MAX_WPA_PASS 32
        char buf[MAX_WPA_PASS];
        #define WPA2_TEMP_FILE "/tmp/ev3duder_wpa2.conf"
        char wpa_passphrase[256];
        char *pass = argv[1];
        while (!pass)
        pass = fgets(buf, MAX_WPA_PASS, stdin);
        snprintf(wpa_passphrase, sizeof wpa_passphrase, "wpa_passphrase '%s' '%s' > '" WPA2_TEMP "'" argv[0], pass);
        exec(wpa_passphrase);
        exec("wpa_supplicant -Dwext -iwlan0 -c" WPA2_TEMP);

        }*/
        return ERR_UNK;
    case ARG_exec:
        assert(argc >= 1);
		//FIXME: >1024 bytes hang the thing; maybe pipe through head -c1000
        size_t len_cmd = strlen(argv[0]),
               len_out = sizeof " &>" OutputName;
        buf = malloc(len_cmd + len_out);
        (void)mempcpy(mempcpy(buf,
                              argv[0], len_cmd),
                      " &>" OutputName, len_out);
        argv[0] = buf;
        len = mkrbf(&buf, argv[0]);
        fp = tmpfile();
        if (!fp)
            return ERR_IO;
        fwrite(buf, len, 1, fp);
        rewind(fp);
        ret = up(fp, ExecName);
        if (ret != ERR_UNK)
            break;
        ret = run(ExecName);
        if (ret != ERR_UNK)
            break;

		fclose(fp);
        break;
    case ARG_rm:
        assert(argc == 1);
        ret = rm(argv[0]);
        break;
    default:
        ret = ERR_ARG;
        printf("<%s> hasn't been implemented yet.\n", argv[0]);
    }

    FILE *out = ret == ERR_UNK ? stderr : stdout;
    if (ret == ERR_COMM)
        fprintf(out, "%s (%ls)\n", errmsg ?: "-", ev3_error(handle) ?: L"-");
    else if (ret == ERR_VM){
        const char *err;
        if (errno < (int)ARRAY_SIZE(ev3_error_msgs))
            err =  ev3_error_msgs[errno];
        else
            err = "An unknown error occured";

        fprintf(out, "%s (%s)\n", err ?: "-", errmsg ?: "-");
    }else {
		if (errmsg) fprintf(out, "%s\n", errmsg);
	}

    // maybe \n to stderr?
    if (handle) ev3_close(handle);
    return ret;
}

#pragma GCC diagnostic ignored "-Wunused"
static char* chrsub(char *s, char old, char new)
{
    char *ptr = s;
    if (ptr == NULL || *ptr == '\0')
        return NULL;
    do
    {
        if (*ptr == old) *ptr = new;
    } while(*++ptr);
    return s;
}

