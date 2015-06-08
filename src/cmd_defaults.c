#include "systemcmd.h"

const BEGIN_DOWNLOAD
BEGIN_DOWNLOAD_INIT = { .hidLayer = 0x00, .replyType = 0x01, .cmd = 0x92 };

const CONTINUE_DOWNLOAD
CONTINUE_DOWNLOAD_INIT = {  .hidLayer = 0x00, .replyType = 0x01, .cmd = 0x93};

const CONTINUE_DOWNLOAD_REPLY
CONTINUE_DOWNLOAD_REPLY_SUCCESS = {.packetLen = 6, .type = 0x03, .cmd = 0x93, .ret = 0x08};

const BEGIN_UPLOAD
BEGIN_UPLOAD_INIT = { .hidLayer = 0x00, .replyType = 0x01, .cmd = 0x94 };

const CONTINUE_UPLOAD
CONTINUE_UPLOAD_INIT = {  .hidLayer = 0x00, .replyType = 0x01, .cmd = 0x95};

const LIST_FILES
LIST_FILES_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x99 };

const CONTINUE_LIST_FILES
CONTINUE_LIST_FILES_INIT = {.hidLayer =0x00, .packetLen =7, .replyType = 0x01, .cmd = 0x9A};

const BEGIN_GETFILE
BEGIN_GETFILE_INIT = {.hidLayer =0x00, .replyType = 0x01, .cmd = 0x96 };

const CREATE_DIR
CREATE_DIR_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x9B };

const DELETE_FILE
DELETE_FILE_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x9C };

const BLUETOOTHPIN
BLUETOOTHPIN_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x9F};

const ENTERFWUPDATE
ENTERFWUPDATE_INIT = {.hidLayer = 0x00, .replyType = 0x00, .cmd = 0xA0};

const EXECUTE_FILE
EXECUTE_FILE_INIT = {.hidLayer = 0x00, .replyType = 0x00, .alloc = 0x0800};

const VM_REPLY
EXECUTE_FILE_REPLY_SUCCESS = {.packetLen = 3, .replyType = 0x02};

