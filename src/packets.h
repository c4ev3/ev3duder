/**
 * @file packets.h
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief packed structs for the packets.
 * @bug A more standard approach like libpack might be worthwhile.
 */

#ifndef EV3DUDER_PACKETS_H
#define EV3DUDER_PACKETS_H

#include "defs.h"

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#error "Big-endian systems not yet supported"
/* This would require byte swapping those u16's below */
#endif


// inheritance :^)
//! Is always zero. Also called HID report ID
#define HID_LAYER \
	u8 hidLayer;

/**
 * packetLen isn't supposed to be handled outside of packet_alloc(), therefore abstracted into a #define
 * msgCount might (?)  be used for checking dropped or out-of-order packets.
 * Which would be pointless as TCP and RFCOMM both guarantee reliability anyway
 */
#define EV3_PACKET_FIELDS \
	u16 packetLen;\
	u16 msgCount;

/**
 * replyType defines whether client demands answer. (Yes he always does)
 * cmd packet id, is set by specifing correct data type in packet_alloc().
 * Manual handling is not intended, therefore in #define
 */
#define EV3_COMMAND_FIELDS \
	HID_LAYER \
	EV3_PACKET_FIELDS \
	u8 replyType;\
	u8 cmd;

/**
 * additionally ret describes the return value for last action
 * @see error.h for how to interpret these codes
 */
#define EV3_REPLY_FIELDS \
	EV3_PACKET_FIELDS \
	u8 type; \
	u8 cmd; \
	u8 ret;

/**
 * bytes not covered by packetLen (3B = 1B hidLayer + 2B packetLen itself)
 * @bug hidLayer shouldn't be maintained here as it's non-sense to demand bluetooth to account for it
 * 		Maybe wrap HIDAPI with a wrapper that prefixes zero automatically, instead of having
 * 		bt_write() omit the first byte
 */
#define PREFIX_SIZE 3

/**
 * Padding might be inserted in order to allow for a favorable alignment
 * This pragma forces the compiler to ensure sizeof(structure) to equal the sizeof(all members)
 * It's not pretty, but it works well enough
 */
#pragma pack(push, 1) //README: __attribute__((packed)) doesn't work for whatever reason

//! base packet for SYSTEM COMMANDS
typedef struct
{
	EV3_COMMAND_FIELDS

	u8 bytes[];
} SYSTEM_CMD;

//!base packet for SYSTEM REPLIES
typedef struct
{
	EV3_REPLY_FIELDS
	u8 bytes[];
} SYSTEM_REPLY;

//! upload to EV3
typedef struct
{
	EV3_COMMAND_FIELDS
	/** size of file in bytes */
	u32 fileSize;
	/** file name, relative to "lms2012/sys", First folder must be apps, prjs or tools according to docs
	 * Ignore that and use proper absolute paths instead 
	 */
	char fileName[];
} BEGIN_DOWNLOAD;
extern const BEGIN_DOWNLOAD BEGIN_DOWNLOAD_INIT;

//! Th ev3's reply
typedef struct
{
	EV3_REPLY_FIELDS
	/** needs to be specified for #CONTINUE_DOWNLOAD s */
	u8 fileHandle;
} BEGIN_DOWNLOAD_REPLY;


//! send file chunk wise
typedef struct
{
	EV3_COMMAND_FIELDS
	/** file handle as recieved by BEGIN_DOWNLOAD_REPLY */
	u8 fileHandle;
	/* Your data */
	char fileChunk[];
} CONTINUE_DOWNLOAD;

extern const CONTINUE_DOWNLOAD CONTINUE_DOWNLOAD_INIT;

typedef BEGIN_DOWNLOAD_REPLY CONTINUE_DOWNLOAD_REPLY;
extern const CONTINUE_DOWNLOAD_REPLY CONTINUE_DOWNLOAD_REPLY_SUCCESS;

//! download from EV3
typedef struct
{
	EV3_COMMAND_FIELDS

	/** maxBytes for a single chunk. Use 1000 */
	u16 maxBytes;
	/** UTF-8 encoded, NUL-terminated string */
	char fileName[];
} BEGIN_UPLOAD;

//! download from EV3 reply
typedef struct
{
	EV3_REPLY_FIELDS

	/** Total file size **/
	u32 fileSize;
	/** Handle for use in successive operations **/
	u8 fileHandle;
	/** First chunk **/
	u8 bytes[];
} BEGIN_UPLOAD_REPLY;

//! continue download from ev3
typedef struct
{
	EV3_COMMAND_FIELDS

	/** recieved in BEGIN_UPLOAD_REPLY **/
	u8 fileHandle;
	/** chunk size **/
	u16 maxBytes;
} CONTINUE_UPLOAD;

//! continue download from ev3 reply
typedef struct
{
	EV3_REPLY_FIELDS

	/** recieved in BEGIN_UPLOAD_REPLY **/
	u8 fileHandle;
	/** successive chunks **/
	u8 bytes[];
} CONTINUE_UPLOAD_REPLY;

extern const CONTINUE_UPLOAD CONTINUE_UPLOAD_INIT;
extern const BEGIN_UPLOAD BEGIN_UPLOAD_INIT;


//! List files on EV3
typedef struct
{
	EV3_COMMAND_FIELDS
	/** doesn't really matter as #CONTINUE_LIST_FILES isn't implemented in stock firmware */
	u16 maxBytes;
	/** path to list files for. relative to VM path */
	u8 path[];
} LIST_FILES;
extern const LIST_FILES LIST_FILES_INIT;

typedef LIST_FILES BEGIN_GETFILE;
extern const BEGIN_GETFILE BEGIN_GETFILE_INIT;

/** 
 * example data:
 * <pre>1EB0CADE3A6C2FD80A96706D4D42114 0000003C run
 * mod/</pre>
 *
 * directories end with a / and then \\n
 * file entries are of 3 parts:
 * 32 bytes of md5 hash
 * the file size in hexadecimal notation
 * the file name and then a \\n, all separated by spaces
 * @warning Data might be cut off. Be sure to check if the line is valid
 * before jumping to any conclusions. This is due to the hard limit 
 * on HIDAPI packet, md5 hashes taking a lot of bytes and CONTINUE_LIST_FILES
 * being unimplemented in VM
 * @brief Directory contents on EV3
 */
typedef struct
{
	EV3_REPLY_FIELDS

	u32 listSize; // when listSize < maxBytes; reached EO List?
	u8 handle; // for CONTINUE_LIST_FILES
	/* files according to above description.
	 * \n separated and UTF-8 encoded
	 */
	char list[];
} LIST_FILES_REPLY;

/** not implemented in stock firmware */
typedef struct
{
	EV3_COMMAND_FIELDS

	u8 handle;
	u32 listSize;
} CONTINUE_LIST_FILES;
extern const CONTINUE_LIST_FILES CONTINUE_LIST_FILES_INIT;

/** not implemented in stock firmware */
typedef struct
{
	EV3_REPLY_FIELDS

	u8 handle; // for CONTINUE_LIST_FILES
	char list[];
	// \n seperated; UTF-8 encoded.
} CONTINUE_LIST_FILES_REPLY;

//! create directory
typedef struct
{
	EV3_COMMAND_FIELDS

	/** @see mkdir() */
	u8 path[];
} CREATE_DIR;

extern const CREATE_DIR CREATE_DIR_INIT;

typedef SYSTEM_REPLY CREATE_DIR_REPLY;

//! delete file
typedef struct
{
	EV3_COMMAND_FIELDS
	/** @see rm() */
	u8 path[];
} DELETE_FILE;
extern const DELETE_FILE DELETE_FILE_INIT;

typedef SYSTEM_REPLY DELETE_FILE_REPLY;

//! set bluetooth pin, untested
typedef SYSTEM_CMD BLUETOOTHPIN;
/*typedef struct
{
	EV3_COMMAND_FIELDS

	u8 mac_len;
	u8 mac[mac_len]; // asciiz hex; no colons
	u8 pin_len;
	u8 pen[pin_len]; // asciiz
} BLUETOOTHPIN;*/ /* implement with mempcpy like in exec.c */

extern const BLUETOOTHPIN BLUETOOTHPIN_INIT;

//! untested and unused
typedef struct
{
	EV3_REPLY_FIELDS

	BLUETOOTHPIN echo; // GNU C extension
} BLUETOOTHPIN_REPLY;

//! Force brick into Firmware update mode, untested and unused
typedef SYSTEM_CMD ENTERFWUPDATE;
extern const ENTERFWUPDATE ENTERFWUPDATE_INIT;

//
/// VM stuff
#define EV3_VM_COMMAND_FIELDS \
	HID_LAYER \
	EV3_PACKET_FIELDS \
	u8 replyType; \
	u16 alloc;

//! base packet
typedef struct
{
	EV3_VM_COMMAND_FIELDS

	u8 bytes[];
} VM_CMD;

typedef VM_CMD EXECUTE_FILE;
extern const EXECUTE_FILE EXECUTE_FILE_INIT;

/*
typedef struct
{
	EV3_VM_COMMAND_FIELDS	

	u8 opCode;
	u8 cmd;
	u8 userSlot;



} EXEC_FILE;

C00882010084 2E2E2F70726A732F42726B50726F675F534156452F44656D6F2E72706600
cccccccccccc cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

*/

//! base reply packet
typedef struct
{
	EV3_PACKET_FIELDS
	u8 replyType;
	u8 bytes[];
} VM_REPLY;

extern const VM_REPLY EXECUTE_FILE_REPLY_SUCCESS;


#pragma pack(pop)

//! for variably sized packets. Allocates space, initializes and adjusts packetLen field
//! \note employs the compound statement expression GNU extension
#define packet_alloc(type, extra) ({                            \
	void *ptr = NULL;                                 \
	if ((sizeof(type) + extra < (u16)-1)              \
	&& (ptr = malloc(sizeof(type) + extra))) {            \
	memcpy(ptr, &type##_INIT, sizeof(type));                    \
	((SYSTEM_CMD *)ptr)->packetLen = (u16)(sizeof(type) + extra - PREFIX_SIZE);                                          \
  }                                                   \
	ptr;})

#endif
