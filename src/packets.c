/**
 * @file packets.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief contains inital values for all these packets
 */
#include "packets.h"

const BEGIN_DOWNLOAD
		BEGIN_DOWNLOAD_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x92};

const CONTINUE_DOWNLOAD
		CONTINUE_DOWNLOAD_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x93};

const CONTINUE_DOWNLOAD_REPLY
		CONTINUE_DOWNLOAD_REPLY_SUCCESS = {.packetLen = 6, .type = 0x03, .cmd = 0x93, .ret = 0x08};

const BEGIN_UPLOAD
		BEGIN_UPLOAD_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x94};

const CONTINUE_UPLOAD
		CONTINUE_UPLOAD_INIT = {.hidLayer = 0x00, .packetLen = sizeof(CONTINUE_UPLOAD) -
															   PREFIX_SIZE, .replyType = 0x01, .cmd = 0x95};

const LIST_FILES
		LIST_FILES_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x99};

const CONTINUE_LIST_FILES
		CONTINUE_LIST_FILES_INIT = {.hidLayer =0x00, .packetLen =7, .replyType = 0x01, .cmd = 0x9A};

const BEGIN_GETFILE
		BEGIN_GETFILE_INIT = {.hidLayer =0x00, .replyType = 0x01, .cmd = 0x96};

const CREATE_DIR
		CREATE_DIR_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x9B};

const DELETE_FILE
		DELETE_FILE_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x9C};

const CLOSE_HANDLE
		CLOSE_HANDLE_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x98};

const BLUETOOTHPIN
		BLUETOOTHPIN_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0x9F};

const ENTERFWUPDATE
		ENTERFWUPDATE_INIT = {.hidLayer = 0x00, .replyType = 0x81, .cmd = 0xA0};

const FW_GETVERSION
		FW_GETVERSION_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0xF6};

const FW_GETCRC32
		FW_GETCRC32_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0xF5};

const FW_STARTAPP
		FW_STARTAPP_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0xF4};

const FW_ERASEFLASH
		FW_ERASEFLASH_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0xF3};

const FW_START_DOWNLOAD_WITH_ERASE
		FW_START_DOWNLOAD_WITH_ERASE_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0xF0};

const FW_START_DOWNLOAD
		FW_START_DOWNLOAD_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0xF1};

const FW_DOWNLOAD_DATA
		FW_DOWNLOAD_DATA_INIT = {.hidLayer = 0x00, .replyType = 0x01, .cmd = 0xF2};

const EXECUTE_FILE
		EXECUTE_FILE_INIT = {.hidLayer = 0x00, .replyType = 0x00, .alloc = 0x0800};

const VM_REPLY
		EXECUTE_FILE_REPLY_SUCCESS = {.packetLen = 3, .replyType = 0x02};
