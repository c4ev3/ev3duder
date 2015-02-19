#include "typedefs.h"

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

#define PREFIX_SIZE 3

#pragma pack(push, 1) //README: __attribute__((packed)) doesn't work for whatever reason

typedef struct
{
	EV3_COMMAND_FIELDS

	u8 bytes[];
} SYSTEM_CMD;

typedef struct
{
	EV3_COMMAND_FIELDS

	u32 fileSize;
	char fileName[];
} BEGIN_DOWNLOAD;
extern BEGIN_DOWNLOAD BEGIN_DOWNLOAD_INIT;

typedef struct __attribute__((packed))
{
	EV3_REPLY_FIELDS

	u8 fileHandle;
} BEGIN_DOWNLOAD_REPLY;



typedef struct __attribute__((packed))
{
	EV3_COMMAND_FIELDS

	u8 fileHandle;
	char fileChunk[];
} CONTINUE_DOWNLOAD;
extern CONTINUE_DOWNLOAD CONTINUE_DOWNLOAD_INIT;

typedef BEGIN_DOWNLOAD_REPLY CONTINUE_DOWNLOAD_REPLY;
extern CONTINUE_DOWNLOAD_REPLY CONTINUE_DOWNLOAD_REPLY_SUCCESS;

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

#pragma pack(pop)
