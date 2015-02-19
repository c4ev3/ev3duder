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
#pragma pack(pop)
