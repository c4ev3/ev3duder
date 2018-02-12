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
#include <errno.h>

#include <hidapi/hidapi.h>

#include "btserial.h"
#include "tcp.h"
#include "ev3_io.h"
#include "error.h"
#include "funcs.h"



static struct
{
	unsigned select:1;
	unsigned hid:1;
	unsigned serial:1;
	unsigned tcp:1;
} switches;

static struct
{
	char *tcp_id;
	char *serial_id[2];
	char *usb_id;
	unsigned timeout;
} params;


static char *my_chrsub(char *s, char old, char new);

//#undef assert
//xFIXME: add better error message
//#define assert(cond) do{ if (!(cond)) {if (handle) ev3_close(handle);exit(ERR_ARG);}}while(0)
void assert(char cond);

#ifdef _WIN32
#define SANITIZE(s) (my_chrsub((s), '/', '\\'))
#else
#define SANITIZE
#endif


//! LEGO GROUP
#define VendorID 0x694
//! EV3
#define ProductID 0x005 /* EV3 */
//! Filename used for executing command with `exec`
#define ExecName  "/tmp/Executing shell cmd.rbf"
// FIXME: general solution

/**
 * suffix for commands executed with exec
 * problem: busybox dd doesn't support conv=sync (zero-padding blocks)
 * solution: prefix with length instead:
 * First pipe stderr to stdout then pipe to awk which prefixes a header
 * with the byte count to follow and then pipe that to  dd which writes
 * to the hid device driver in chunks of 1000 bytes. 
 * This is necessary as bs>1024 causes the system to stall 
 * (driver buffer overflow maybe).
 * unescaped version:
 * echo "hey" 2>&1 | awk 'BEGIN{RS="\0"};{printf "%08x%s", length($
0), $0}' | dd bs=1000 of=/dev/lms_usbdev
 */
static const char EXEC_SUFFIX[] =
/*cmd*/    		" 2>&1 | awk '"
				"BEGIN{"
				"RS=\"\\0\""
				"};"
				"{"
				"printf \"\\t%08x\\n%s\","
				"length($0),$0"
				"}"
				"' | dd bs=1000 of=/dev/lms_usbdev";

const char *const usage =
		"USAGE: ev3duder " "[ --tcp[=ip] | --usb | --serial[=dev] | --serial[=inp,outp] ] \n"
				"                " "[ up loc rem | dl rem loc | rm rem | ls [rem] |\n"
				"                " "  mkdir rem | mkrbf rem loc | run rem | exec cmd |\n"
#ifdef BRIDGE_MODE
				"                " "  wpa2 SSID [pass] | info | tunnel | bridge ]\n"
#else
#ifdef WPA2_MODE
				"                " "  wpa2 SSID [pass] | info | tunnel ]\n"
#else
				"                " "  info | tunnel ]\n"
#endif
#endif
				"       "
				"rem = remote (EV3) path, loc = local path"    "\n";


const char *const usage_desc = "Info:\n"
		"--help | -h | /? Show this help\n"
		"--version       Print version number\n"
		"--printexits    Print a list of exit codes\n"
		"\n"
		"Parameters:\n"
		"--tcp[=ip]      Connect to the EV3 over TCP. If no ip is specified,\n"
		"                the auto-detection feature will be used\n"
		"--usb           Connect to the EV3 over USB\n"
		"--serial[=dev]  Connect to the EV3 over a serial port\n"
		"--serial[=i,o]  Connect to the EV3 over a serial port,\n"
		"                with different files for input and output streams\n"
		"\n"
		"Commands:\n"
		" up             upload local file to remote ev3 brick\n"
		" dl             download remote file to local system\n"
		" rm             remove file on ev3 brick\n"
		" ls             list files. Standard value is '/'\n"
		" info           attempt a beep and print information about the connection\n"
		" mkdir          create directory on the remote filesystem. Relative to path of VM.\n"
		" mkrbf          create rbf (menu entry) file locally. Sensible upload paths are:\n"
		"                ../prjs/BrkProg_SAVE/   Internal memory\n"
		"                ../prjs/BrkProg_DL/     Internal memory\n"
		"                ./apps/                 Internal memory - Applicatios (3rd tab)\n"
		"                /media/card/myapps/     Memory card\n"
		"                /media/usb/myappps/     USB stick\n"
		"run             instruct the VM to run a rbf file\n"
		"exec            pass cmd to root shell. Handle with caution\n"
		"tunnel          connects stdout/stdin to the ev3 VM\n"
#ifdef WPA2_MODE
		"wpa2            connect to WPA-Network SSID, if pass isn't specified, read from stdin\n"
#endif
#ifdef BRIDGE_MODE
		"bridge          simulates a WiFi-connected device bridged to a real ev3 VM\n"
#endif
;

#define FOREACH_ARG(ARG)\
    ARG(info)			\
	ARG(up)				\
	ARG(dl)				\
	ARG(run)			\
	ARG(ls)				\
	ARG(rm)				\
	ARG(mkdir)			\
	ARG(mkrbf)			\
	ARG(tunnel)			\
	ARG(bridge)			\
	ARG(listen)			\
	ARG(send)			\
	ARG(exec)			\
	ARG(wpa2)			\
	ARG(nop)			\
	ARG(end)


#define MK_ENUM(x) ARG_##x,
#define MK_STR(x) #x,
enum ARGS
{
	FOREACH_ARG(MK_ENUM)
};
static const char *args[] = {FOREACH_ARG(MK_STR)};
static const char *offline_args[] = {MK_STR(mkrbf) /*MK_STR(tunnel)*/ };




int main(int argc, char *argv[])
{
	handle = NULL;

	// Handle -h/--help params
	if (argc == 2) {
		if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "/?") == 0)
		{
			printf("%s (%s; %s) v%s\n"
						   "Copyright (C) 2016 Ahmad Fatoum\n"
						   "This is free software; see the source for copying conditions.   There is NO\n"
						   "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
						   "Source is available under the GNU GPL v3.0 https://github.com/c4ev3/ev3duder/\n\n",
				   argv[0], CONFIGURATION, SYSTEM, VERSION);
			puts(usage);
			puts(usage_desc);
			return ERR_UNK;
		}
		else if (strcmp(argv[1], "--printexits") == 0)
		{
			printf("Exit status:\n");
			printf("%d  ERR_ARG     Wrong argument\n", ERR_ARG);
			printf("%d  ERR_IO      IO operation failed\n", ERR_IO);
			printf("%d  ERR_FTOOBIG File too big\n", ERR_FTOOBIG);
			printf("%d  ERR_NOMEM   No memory available\n", ERR_NOMEM);
			printf("%d  ERR_COMM    Communication failed\n", ERR_COMM);
			printf("%d  ERR_VM      EV3 VM returned an error\n", ERR_VM);
			printf("%d  ERR_SYS     System error\n", ERR_SYS);
			printf("%d  ERR_END     \n", ERR_END);

			return ERR_UNK;
		}
		else if (strcmp(argv[1], "--version") == 0)
		{
			printf("%s\n", VERSION);
			return ERR_UNK;
		}
	}

	while (argv[1] && *argv[1] == '-')	// Handle parameters and flags starting with - and --
	{

		if (argv[1][1] == '-')	// Handle -- parameters
		{   /* switches */
			char *a = argv[1] + 2;	// Skip the --
			strtok(a, "=");
			char *device = strtok(NULL, ","),
					*device2 = strtok(NULL, "");

			if (a == '\0')
			{
				argc--, argv++;
				break;
			}

			if (strcmp("usb", a) == 0 || strcmp("hid", a) == 0)
			{
				switches.select = switches.hid = 1;
				params.usb_id = device;
			}
			else if (strcmp("tcp", a) == 0 || strcmp("inet", a) == 0)
			{
				switches.select = switches.tcp = 1;
				params.tcp_id = device;
			}
			else if (strcmp("serial", a) == 0 || strcmp("bt", a) == 0)
			{
				switches.select = switches.serial = 1;
				params.serial_id[0] = device;
				params.serial_id[1] = device2;
			}
			else if (strcmp("nop", a) == 0)
			{
				/* So, this program doesn't handle empty *argv, which
					 is understandable, but would ease programming the
					 plugin a bit. So let the plugin just specify
					 --nop. It's just a semicolon
				*/
			}
			else    // Unknown prameter starting with --
			{
				fprintf(stderr, "Invalid switch '%s'\n", argv[1]);
				return ERR_ARG;
			}

			argc--, argv++;
		}
		else if (argv[1][1] == 't')	// Handle parameters starting with - (-t)
		{
			// we don't care about negative numbers as any non-integer
			// means indefinitely. negative numbers are so big they are
			// practically infinite
			if (argv[1][2] == '=')
				params.timeout = atoi(&argv[1][3]);
			else
			{
				params.timeout = atoi(argv[2]);
				argv++, argc--;
			}
			argv++, argc--;
		}
		else	// Invalid parameter starting with -
		{
			fprintf(stderr, "Invalid parameter '%s'\n", argv[1]);
			return ERR_ARG;
		}

	}

	if (argc == 1)	// After parameter parsing no command parameter is left
	{
		puts(usage);
		puts(usage_desc);
		return ERR_ARG;
	}

	// Determine if the command is an offline command
	int i;
	for (i = 0; i < (int) ARRAY_SIZE(offline_args); i++) {
		if (strcmp(argv[1], offline_args[i]) == 0) break;
	}

	// If the command is not one of the offline commands, select transfer method
	if (i == ARRAY_SIZE(offline_args))
	{
		if ((switches.hid || !switches.select) &&
			(handle = hid_open(VendorID, ProductID, NULL)))
			// TODO: last one is SerialID, make it specifiable via commandline
		{
			if (!switches.select)
				fputs("USB connection established.\n", stderr);
			// the things you do for type safety...

			ev3_write = (int (*)(void *, const u8 *, size_t)) hid_write;
			ev3_read_timeout = (int (*)(void *, u8 *, size_t, int)) hid_read_timeout;
			ev3_error = (const wchar_t *(*)(void *)) hid_error;
			ev3_close = (void (*)(void *)) hid_close;
		}
		else if ((switches.serial || !switches.select) && (handle = bt_open(params.serial_id[0], params.serial_id[1])))
		{
			if (!switches.select)
				fprintf(stderr, "Bluetooth serial connection established (%s).\n", params.serial_id[0]);

			ev3_write = bt_write;
			ev3_read_timeout = bt_read;
			ev3_error = bt_error;
			ev3_close = bt_close;
		}
		else if ((switches.tcp || !switches.select) && (handle = tcp_open(params.tcp_id, params.timeout)))
		{
			if (!switches.select)
			{
				struct tcp_handle *info = (struct tcp_handle *) handle;
				fprintf(stderr, "TCP connection established (%s@%s:%u).\n", info->name, info->ip, info->tcp_port);
			}

			ev3_write = tcp_write;
			ev3_read_timeout = tcp_read;
			ev3_error = tcp_error;
			ev3_close = tcp_close;
		}
		else
		{
			puts("EV3 not found. Either plug it into the USB port or pair over Bluetooth.\n");
#ifdef __linux__
			puts("Insufficient access to the usb device might be a reason too, try sudo.");
#endif
			return ERR_COMM;
		}
	}

	// Find out what actual command is given
	for (i = 0; i < ARG_end; i++)
		if (strcmp(argv[1], args[i]) == 0) break;

	// Move 2 arguments forwards. Now using argv[0] instead of argv[1]
	argc -= 2;
	argv += 2;

	int ret;
	FILE *fp = NULL;
	char *buf = NULL;
	size_t len = 0;

	// Switch between the different commands
	switch (i)
	{
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
			}
			else buf = argv[1];

			fp = fopen(buf, "wb"); //TODO: no sanitize here?
			if (!fp)
			{
				printf("File <%s> couldn't be opened for writing.\n", buf);
				return ERR_IO;
			}

			ret = dl(argv[0], fp);
			break;

		case ARG_run:
			assert(argc == 1);
			ret = run(argv[0], params.timeout);
			break;

		case ARG_nop: // just connect and do nothing
			return ERR_UNK;

		case ARG_end:
			puts(usage);
			return ERR_ARG;

		case ARG_ls:
			assert(argc <= 1);
			ret = ls(argv[0] ?: "/");
			break;

		case ARG_tunnel:
			ret = tunnel_mode();
			break;

		case ARG_listen:
			ret = listen_mode();
			break;

		case ARG_bridge:
#ifdef BRIDGE_MODE
			ret = bridge_mode();
#else
			puts(usage);
			return ERR_ARG;
#endif
			break;

		case ARG_send:;
			int sendout(void);
			ret = sendout();
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

#ifdef WPA2_MODE
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
#endif
		case ARG_exec:
			assert(argc >= 1);
			size_t len_cmd = strlen(argv[0]),
					len_out = sizeof EXEC_SUFFIX;
			buf = malloc(len_cmd + len_out);
			(void) mempcpy(mempcpy(buf,
					   argv[0], len_cmd),
					   EXEC_SUFFIX, len_out);
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
			ret = run(ExecName, params.timeout);
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
			break;
	}


	FILE *out = ((ret == ERR_UNK) ? stderr : stdout);
	if (ret == ERR_COMM)
		fprintf(out, "%s (%ls)\n", errmsg ?: "-", ev3_error(handle) ?: L"-");

	else if (ret == ERR_VM)
	{
		const char *err;
		if (errno < (int) ARRAY_SIZE(ev3_error_msgs))
			err = ev3_error_msgs[errno];
		else
			err = "An unknown error occured";

		fprintf(out, "%s (%s)\n", err ?: "-", errmsg ?: "-");
	}
	else
	{
		if (errmsg) fprintf(out, "%s\n", errmsg);
	}

// maybe \n to stderr?
	if (handle) ev3_close(handle);

	return ret;
}

#pragma GCC diagnostic ignored "-Wunused-function"

static char *my_chrsub(char *s, char old, char new)
{
	char *ptr = s;
	if (ptr == NULL || *ptr == '\0')
		return NULL;
	do
	{
		if (*ptr == old) *ptr = new;
	} while (*++ptr);
	return s;
}

void assert(char cond)
{
	if (!cond) {
		if (handle) ev3_close(handle);
		exit(ERR_ARG);
	}
}
