#include "systemcmd.h"

BEGIN_DOWNLOAD
BEGIN_DOWNLOAD_INIT = { .hidLayer = 0x00, .replyType = 0x01, .cmd = 0x92 };

CONTINUE_DOWNLOAD
CONTINUE_DOWNLOAD_INIT = {  .hidLayer = 0x00, .replyType = 0x01, .cmd = 0x93};

CONTINUE_DOWNLOAD_REPLY
CONTINUE_DOWNLOAD_REPLY_SUCCESS = {.packetLen = 6, .type = 0x03, .cmd = 0x93, .ret = 0x08};
