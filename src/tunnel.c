/**
 * @file tunnel.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief tunnel stdio to established ev3 connection
 * @see rc.pl for a usage scenario
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "ev3_io.h"

#include "defs.h"
#include "packets.h"
#include "error.h"
#include "funcs.h"

#ifdef _WIN32
#define isatty _isatty
#endif

static int hex2nib(char hex);

/**
 * \retval err_code An error code according to `enum #ERR`
 * \brief tunnels from stdio to ev3
 *
 *	- If stdio is connected to a terminal, packets can be entered in hex and submitted with a line break. non-graphical characters, including whitespace, are ignored. If the first 4 bytes can't be parsed as hex digits, they are replaced with the binary length. Any other character is replaced with a nibble rounded up.
 *	- If stdio is a pipe, the first 2 bytes are read to acquire packet length and then an equal amount of bytes is read and sent to the ev3
 */
int tunnel_mode()
{
	int res;
	(void)res;
	u8 binbuf[1280];
	binbuf[0] = 0x00;
#if 0
	struct termios new, old;
	tcgetattr(0, &old);
	new = old;
	new.c_lflag &= ~ICANON; /* disable buffered i/o */
    new.c_lf lag &= ~ECHO; /* disable echoing input */
    tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
#endif
	if (isatty(STDIN_FILENO) || 1)
	{
		char hexbuf[2560];
		while (fgets(hexbuf, sizeof hexbuf, stdin))
		{
			int prefix_len = 0;
			const char *hp; u8 *bp;
			int tmp;
			for (hp = hexbuf, bp = binbuf+1; *hp != '\0'; hp++)
			{
				if ((tmp = hex2nib(*hp)) == -1) continue;
				if (tmp == 0 && *hp != '0')
				{	
					prefix_len++;
					if (prefix_len == 4)
						break;
				}else break;
			}

			u8 byte = 0; int nibble = 0;
			for (hp = hexbuf, bp = binbuf+1; *hp != '\0'; hp++)
			{
				if ((tmp = hex2nib(*hp)) == -1) continue;
				//printf("hp=[%c],tmp=%d\n", *hp, tmp);
				if (nibble == 1)
				{
					*bp++ = byte + tmp;
					nibble = 0;
				}else
				{
					byte = (u8)tmp << 4;
					nibble = 1;
				}
			}

			int len = bp - binbuf+1; //FIXME: size_t
			if(prefix_len == 4)
				memcpy(binbuf+1, (u16[]){len - 2}, 2);
			printf("prefix_len=%d, len=%i\n", prefix_len, len);

			// ev3_write then read
			print_bytes(binbuf, len - 1);
			ev3_write(handle, binbuf, len -1);
		}
	}
	else{
	/* FIXME: switch to binary setmode */
		printf("%zu\n", fread(binbuf+1, 1, 2, stdin));
		size_t len = (binbuf[1] | (binbuf[2] << 4));
		printf("len%zu\n", len);
		fread(binbuf+3, 1, len, stdin);
		print_bytes(binbuf, (int)len+3);

	}


	return ERR_UNK;
}
static int hex2nib(char hex)
{
	/**/ if ('a' <= hex && hex <= 'f')
		return hex - 'a' + 0x0a;
	else if ('A' <= hex && hex <= 'F')
		return hex - 'A' + 0x0a;
	else if ('0' <= hex && hex <= '9')
		return hex - '0';
	else if (isspace(hex) || !isgraph(hex))
		return -1;
	else
		return 0;
}

