#include "defs.h"
// inheritance :^)
#define HID_LAYER \
	u8 hidLayer;

#define EV3_PACKET_FIELDS \
	u16 packetLen;\
	u16 msgCount;

#define EV3_COMMAND_FIELDS \
	HID_LAYER \
	EV3_PACKET_FIELDS \
	u8 replyType;\
	u8 cmd;

#define EV3_REPLY_FIELDS \
	EV3_PACKET_FIELDS \
	u8 type; \
	u8 cmd; \
	u8 ret;

#define PREFIX_SIZE 3 // bytes not covered by packetLen (3B = 1B hidLayer + 2B packetLen itself)

#pragma pack(push, 1) //README: __attribute__((packed)) doesn't work for whatever reason

typedef struct
{
	EV3_COMMAND_FIELDS

	u8 bytes[];
} SYSTEM_CMD;
typedef struct
{
  EV3_REPLY_FIELDS
  u8 bytes[];
} SYSTEM_REPLY;

/// upload to EV3
typedef struct
{
	EV3_COMMAND_FIELDS

	u32 fileSize;
	char fileName[]; // relative to "lms2012/sys", First folder must be apps, prjs or tools
} BEGIN_DOWNLOAD;
extern BEGIN_DOWNLOAD BEGIN_DOWNLOAD_INIT;

typedef struct
{
	EV3_REPLY_FIELDS

	u8 fileHandle;
} BEGIN_DOWNLOAD_REPLY;



typedef struct
{
	EV3_COMMAND_FIELDS

	u8 fileHandle;
	char fileChunk[];
} CONTINUE_DOWNLOAD;

extern CONTINUE_DOWNLOAD CONTINUE_DOWNLOAD_INIT;

typedef BEGIN_DOWNLOAD_REPLY CONTINUE_DOWNLOAD_REPLY;
extern CONTINUE_DOWNLOAD_REPLY CONTINUE_DOWNLOAD_REPLY_SUCCESS;

/// List files on EV3
typedef struct
{
    EV3_COMMAND_FIELDS

    u16 maxBytes;
    u8 path[];
} LIST_FILES;
extern LIST_FILES LIST_FILES_INIT;

typedef struct
{
	EV3_REPLY_FIELDS

    u32 listSize; // when listSize < maxBytes; reached EO List?
    u8 handle; // for CONTINUE_LIST_FILES 
    char list[]; 
    // \n seperated; UTF-8 encoded. 
} LIST_FILES_REPLY;

typedef struct
{
    EV3_COMMAND_FIELDS

    u8 handle;
    u32 listSize;
} CONTINUE_LIST_FILES; 

/// create directory

typedef struct
{
    EV3_COMMAND_FIELDS

    u8 path[];
} CREATE_DIR; 

extern CREATE_DIR CREATE_DIR_INIT;

typedef SYSTEM_REPLY CREATE_DIR_REPLY;

// delete file

typedef struct
{
    EV3_COMMAND_FIELDS

    u8 path[];
} DELETE_FILE; 
extern DELETE_FILE DELETE_FILE_INIT;

typedef SYSTEM_REPLY DELETE_FILE_REPLY;

/// set bluetooth pin

typedef SYSTEM_CMD BLUETOOTHPIN;
/*typedef struct
{
    EV3_COMMAND_FIELDS

    u8 mac_len;
    u8 mac[mac_len]; // asciiz hex; no colons
    u8 pin_len;
    u8 pen[pin_len]; // asciiz
} BLUETOOTHPIN;*/ /* implement with mempcpy like in exec.c */ 

extern BLUETOOTHPIN BLUETOOTHPIN_INIT;

typedef struct{
  EV3_REPLY_FIELDS

  BLUETOOTHPIN echo; // GNU C extension
} BLUETOOTHPIN_REPLY;

/// Force brick into Firmware update mode

typedef SYSTEM_CMD ENTERFWUPDATE; 
extern ENTERFWUPDATE ENTERFWUPDATE_INIT;

//no reply

/// VM stuff
#define EV3_VM_COMMAND_FIELDS \
	HID_LAYER \
	EV3_PACKET_FIELDS \
	u8 replyType; \
	u16 alloc;
	 
typedef struct
{
	EV3_VM_COMMAND_FIELDS

	u8 bytes[];
} VM_CMD;

typedef VM_CMD EXECUTE_FILE;
extern EXECUTE_FILE EXECUTE_FILE_INIT;

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

typedef struct
{
	EV3_PACKET_FIELDS
	u8 replyType;
	u8 bytes[];
} VM_REPLY;

extern VM_REPLY EXECUTE_FILE_REPLY_SUCCESS;


#pragma pack(pop)
// for variably sized packets. Allocates space, initializes and adjusts packetLen field
// note: this V is a GNU extension (Compund statement expression or something)
#define packet_alloc(type, extra) ({ 					\
   	void *ptr = malloc(sizeof(type) + extra); 				\
   	memcpy(ptr, &type##_INIT, sizeof(type));					\
	((SYSTEM_CMD *)ptr)->packetLen = sizeof(type) + extra - PREFIX_SIZE;	\
	ptr;})

