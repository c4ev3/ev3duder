/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/*
 * Note: This is a heavily stripped down and edited version of
 * https://github.com/mindboards/ev3sources/blob/master/lms2012/c_com/source/c_wifi.c
 */

#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>s

#include "ev3_io.h"
#include "error.h"

#define NDEBUG

#define BROADCAST_ADDRESS "127.0.0.1"
#define BROADCAST_PORT 3015
#define TCP_PORT 5555
#define BEACON_TIME 1
#define BRICK_NAME "USB Bridge"

typedef enum
{
	TCP_NOT_CONNECTED,    // Waiting for the PC to connect via TCP
	TCP_CONNECTED,        // We have a TCP connection established
	CLOSED                // UDP/TCP closed
} NET_STATE;

typedef enum
{
	TCP_IDLE = 0x00,
	TCP_WAIT_ON_START = 0x01,
	TCP_WAIT_ON_LENGTH = 0x02,
	TCP_WAIT_ON_ONLY_CHUNK = 0x08,
} TCP_CSTATE;

static struct
{
	NET_STATE NetConnection;
	TCP_CSTATE TcpRead;
} State = {CLOSED, TCP_IDLE};

// UDP
static int UdpSocketDescriptor = 0;
static int TcpConnectionSocket = 0; /*  connection socket         */
static int TCPListenServer = 0;
static uint32_t TcpReadBufPointer = 0;
static uint32_t TcpTotalLength = 0;
static uint32_t TcpRestLen = 0;

#ifndef NDEBUG
#define debug_printf(fmt, ...) do { \
		fprintf(stderr, "%s:%i: " fmt "  \t[%s]\n", \
				__FILE__, __LINE__, ##__VA_ARGS__, __FUNCTION__); \
	} while(0)
#else
#define debug_printf(...)
#endif

#define setState(x, y) do { if ((State.x) != (y)) {                      \
			debug_printf("⎯⎯⎯⎯⎯⎯ %s => %s", #x, #y); (State.x) = (y); }} while(0)


// ******************************************************************************

static time_t start = 0;

static void cNetStartTimer()
{
	start = time(NULL);
}

static unsigned cNetCheckTimer()
{
	return time(NULL) - start;
}

static void cNetTcpClose()
{
	int res;
	uint8_t buffer[128];

	struct linger so_linger;

	so_linger.l_onoff = 1;
	so_linger.l_linger = 0;
	setsockopt(TcpConnectionSocket, SOL_SOCKET, SO_LINGER, &so_linger,
			   sizeof so_linger);

	if (shutdown(TcpConnectionSocket, 2) < 0)
	{
		do
		{
			debug_printf("In the do_while");
			res = read(TcpConnectionSocket, buffer, 100);
			if (res < 0) break;
		} while (res != 0);
		debug_printf("Error calling Tcp shutdown()");
	}

	TcpConnectionSocket = 0;
	setState(NetConnection, TCP_NOT_CONNECTED);
}

static void cNetUdpClose(void)
{
	setState(NetConnection, CLOSED);
	close(UdpSocketDescriptor);
}

static bool cNetInitTcpServer()
{
	setState(TcpRead, TCP_IDLE);
	debug_printf("TCPListenServer = %d ", TCPListenServer);

	if (TCPListenServer == 0)
	{
		debug_printf("TCPListenServer == 0");

		if ((TCPListenServer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			debug_printf("Error creating listening socket");
			return false;
		}

		struct sockaddr_in servaddr = {};
		servaddr.sin_family = AF_INET;                // IPv4
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any address
		servaddr.sin_port = htons(TCP_PORT);

		int Temp = fcntl(TCPListenServer, F_GETFL, 0);

		fcntl(TCPListenServer, F_SETFL, Temp | O_NONBLOCK);

		int SetOn = 1;
		Temp = setsockopt(TCPListenServer, SOL_SOCKET, SO_REUSEADDR,
						  &SetOn, sizeof(SetOn));

		if (bind(TCPListenServer, (struct sockaddr *) &servaddr,
				 sizeof(servaddr)) < 0)
		{
			debug_printf("Error calling bind(), errno = %i", errno);
			return false;
		}
	}

	if (listen(TCPListenServer, 1) < 0)
	{
		debug_printf("Error calling listen()");
		return false;
	}

	debug_printf("WAITING for a CLIENT.......");
	return true;
}

static bool cNetWaitForTcpConnection(void)
{
	// blindly wait for up to 1 second
	struct timeval ReadTimeVal;
	ReadTimeVal.tv_sec = 1;
	ReadTimeVal.tv_usec = 0;
	fd_set ReadFdSet;
	FD_ZERO(&ReadFdSet);
	FD_SET(TCPListenServer, &ReadFdSet);
	select(TCPListenServer + 1, &ReadFdSet, NULL, NULL, &ReadTimeVal);

	static struct sockaddr_in servaddr;
	unsigned size = sizeof(servaddr);
	TcpConnectionSocket = accept(TCPListenServer,
								 (struct sockaddr *) &servaddr,
								 &size);

	if (TcpConnectionSocket < 0) return false;

	setState(TcpRead, TCP_WAIT_ON_START);
	debug_printf("Connected to %s:%d",
				 inet_ntoa(servaddr.sin_addr),
				 ntohs(servaddr.sin_port));
	return true;
}

static void hexdump(uint8_t *buf, unsigned len)
{
	const unsigned bpl = 8;
	const unsigned spc = 2;
	char out[bpl * 4 + spc + 1];
	int outpos = 0;
	memset(out, ' ', sizeof(out));
	out[sizeof(out) - 1] = 0;

	for (unsigned i = 0; i < len; i++)
	{
		sprintf(out + outpos, "%02x ", buf[i]);
		char c = buf[i];
		if (!isgraph(c)) c = '.';
		out[(bpl * 3) + spc + (outpos / 3)] = c;
		outpos += 3;
		if (i % bpl == (bpl - 1) || i + 1 == len)
		{
			out[outpos] = ' ';
			debug_printf("%04x  %s", i - bpl + 1, out);
			memset(out, ' ', sizeof(out) - 1);
			outpos = 0;
		}
	}
}

static uint32_t cNetWriteTcp(uint8_t *Buffer, uint32_t Length)
{
	if (Length < 1) return 0;
	if (State.NetConnection != TCP_CONNECTED) return 0;

	hexdump(Buffer, Length);
	return write(TcpConnectionSocket, Buffer, Length);
}

static uint32_t cNetReadTcp(uint8_t *Buffer, uint32_t Length)
{
	int DataRead = 0; // Nothing read also sent if NOT initiated
	switch (State.TcpRead)
	{
		case TCP_IDLE: // Do Nothing
			break;

		case TCP_WAIT_ON_START:
			debug_printf("TCP_WAIT_ON_START:");

			DataRead = read(TcpConnectionSocket, Buffer, 100); // Fixed TEXT

			debug_printf("DataRead = %d, Buffer = ", DataRead);
			hexdump(Buffer, DataRead);

			if (DataRead == 0)
			{
				cNetTcpClose();
				break;
			}

			if (strstr((char *) Buffer, "ET /target?sn="))
			{
				debug_printf("TCP_WAIT_ON_START and ET /target?sn= found.");

				// A match found => UNLOCK
				// Say OK back
				cNetWriteTcp((uint8_t *) "Accept:EV340\r\n\r\n", 16);
				setState(TcpRead, TCP_WAIT_ON_LENGTH);
			}

			DataRead = 0; // No COM-module activity yet
			break;

		case TCP_WAIT_ON_LENGTH:
			debug_printf("TCP_WAIT_ON_LENGTH:");
			assert(Length >= 2 && "Buffer too small");

			TcpReadBufPointer = 0; // Begin on new buffer

			DataRead = read(TcpConnectionSocket, Buffer, 2);

			debug_printf("DataRead = %d", DataRead);
			if (DataRead < 2)
			{
				cNetTcpClose();
				break;
			}

			TcpRestLen = (uint32_t) (Buffer[0] + Buffer[1] * 256);
			TcpTotalLength = (uint32_t) (TcpRestLen + 2);
			setState(TcpRead, TCP_WAIT_ON_ONLY_CHUNK);

			TcpReadBufPointer += DataRead;
			DataRead = 0; // Signal NO data yet

			debug_printf("*************** NEW TX *************");
			debug_printf("TcpRestLen = %d, Length = %d", TcpRestLen, Length);

			break;

		case TCP_WAIT_ON_ONLY_CHUNK:
			debug_printf("TCP_WAIT_ON_ONLY_CHUNK: BufferStart = %d",
						 TcpReadBufPointer);
			assert(Length >= TcpTotalLength && "Buffer too small!");

			DataRead = read(TcpConnectionSocket,
							&(Buffer[TcpReadBufPointer]),
							TcpRestLen);

			debug_printf("DataRead = %d", DataRead);
			debug_printf("BufferPointer = %p", &(Buffer[TcpReadBufPointer]));

			if (DataRead == 0)
			{
				cNetTcpClose();
				break;
			}

			TcpReadBufPointer += DataRead;

			if (TcpRestLen == (unsigned) DataRead)
			{
				DataRead = TcpTotalLength; // Total count read
				setState(TcpRead, TCP_WAIT_ON_LENGTH);
			}
			else
			{
				TcpRestLen -= DataRead; // Still some bytes in this chunk
				DataRead = 0; // No COMM job yet
			}

			hexdump(Buffer, TcpTotalLength);

			debug_printf("TcpRestLen = %d, DataRead = %d, Length = %d",
						 TcpRestLen, DataRead, Length);
			break;

		default: // Should never go here....
			setState(TcpRead, TCP_IDLE);
			break;
	}
	return DataRead;
}


static void cNetTransmitBeacon(void)
{
	if (cNetCheckTimer() < BEACON_TIME) return;

	char Buffer[1024];

	static struct sockaddr_in ServerAddr;
	memset(&ServerAddr, 0, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(BROADCAST_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(BROADCAST_ADDRESS);

	sprintf(Buffer,
			"Serial-Number: %s\r\nPort: %d\r\nName: %s\r\nProtocol: "
					"EV3\r\n",
			"31337", TCP_PORT, BRICK_NAME);

	int UdpTxCount = sendto(UdpSocketDescriptor, Buffer, strlen(Buffer), 0,
							(struct sockaddr *) &ServerAddr, sizeof(ServerAddr));
	debug_printf("UDP BROADCAST to port %d, address %s => %d",
				 ntohs(ServerAddr.sin_port),
				 inet_ntoa(ServerAddr.sin_addr),
				 UdpTxCount);
	if (UdpTxCount < 0) cNetUdpClose();
	cNetStartTimer();
}

static bool cNetInitUdp(void)
{
	/* Get a socket descriptor for UDP client (Beacon) */
	if ((UdpSocketDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		debug_printf("UDP socket() error");
		return false;
	}

	debug_printf("UDP socket() OK, Broadcast to %s", BROADCAST_ADDRESS);
	static int BroadCast = 1;
	if (setsockopt(UdpSocketDescriptor, SOL_SOCKET,
				   SO_BROADCAST, &BroadCast, sizeof(BroadCast)) < 0)
	{
		debug_printf("Could not setsockopt SO_BROADCAST");
		return false;
	}

	int Temp = fcntl(UdpSocketDescriptor, F_GETFL, 0);
	fcntl(UdpSocketDescriptor, F_SETFL, Temp | O_NONBLOCK);

	cNetTransmitBeacon();
	setState(NetConnection, TCP_NOT_CONNECTED);

	return true;
}

int bridge_mode()
{
	if (!cNetInitUdp()) return ERR_IO;
	if (!cNetInitTcpServer()) return ERR_IO;

	static uint8_t ReadBuf[65540] = {0};
	static uint8_t WriteBuf[65540] = {0};
	int write_len = 0, write_pos = 0;

	while (State.NetConnection != CLOSED)
	{
		switch (State.NetConnection)
		{
			case TCP_NOT_CONNECTED:
				cNetTransmitBeacon();

				if (cNetWaitForTcpConnection())
				{
					setState(NetConnection, TCP_CONNECTED);
				}
				break;

			case TCP_CONNECTED:
			{
				if (write_len <= 0)
				{
					write_len = ev3_read_timeout(handle, WriteBuf, sizeof(WriteBuf), 0);
					write_pos = 0;
					if (write_len)
					{
						int realLen = WriteBuf[0] + 256 * WriteBuf[1] + 2;
						assert(realLen <= write_len);
						write_len = realLen;
						debug_printf("ev3_read returned %i", write_len);
					}
				}

				struct timeval timeout;
				timeout.tv_sec = 0;
				timeout.tv_usec = 100000;
				fd_set ReadFdSet, WriteFdSet;
				FD_ZERO(&ReadFdSet);
				FD_SET(TcpConnectionSocket, &ReadFdSet);
				FD_ZERO(&WriteFdSet);
				if (write_len > 0) FD_SET(TcpConnectionSocket, &WriteFdSet);

				select(TcpConnectionSocket + 1, &ReadFdSet, &WriteFdSet, NULL, &timeout);

				if (FD_ISSET(TcpConnectionSocket, &ReadFdSet))
				{
					int len = cNetReadTcp(ReadBuf + 1, sizeof(ReadBuf) - 1);
					if (len > 0)
					{
						int err = ev3_write(handle, ReadBuf, len + 1);
						debug_printf("ev3_write for %i returned %i", len, err);
						if (err < len + 1)
						{
							cNetTcpClose();
							break;
						}
					}
				}
				if (FD_ISSET(TcpConnectionSocket, &WriteFdSet))
				{
					int written = cNetWriteTcp(WriteBuf + write_pos, write_len);
					if (written > 0)
					{
						write_len -= written;
						write_pos += written;
					}
				}

				break;
			}
			default:
				break;
		}
	}

	cNetTcpClose();
	cNetUdpClose();
	return ERR_UNK;
}
