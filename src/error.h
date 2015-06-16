#ifndef EV3DUDER_ERROR_H
#define EV3DUDER_ERROR_H
#undef EXTERN
#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include "defs.h"
enum ERR {ERR_UNK = 0, ERR_ARG, ERR_IO, ERR_FTOOBIG, ERR_NOMEM, ERR_HID, ERR_VM, ERR_SYS, ERR_END};
enum {VM_OK = 0x03, VM_ERROR = 0x05};

EXTERN const char *errmsg;
EXTERN const wchar_t *hiderr;

enum {
    SUCCESS = 0,
    UNKNOWN_HANDLE,
    HANDLE_NOT_READY,
    CORRUPT_FILE,
    NO_HANDLES_AVAILABLE,
    NO_PERMISSION,
    ILLEGAL_PATH,
    FILE_EXITS,
    END_OF_FILE,
    SIZE_ERROR,
    UNKNOWN_ERROR,
    ILLEGAL_FILENAME,
    ILLEGAL_CONNECTION,

    ERRORS_END
};
EXTERN const wchar_t * const ev3_error_msgs[ERRORS_END + 1]
#ifdef MAIN
= {
    [SUCCESS] 		        = L"SUCCESS",
    [UNKNOWN_HANDLE] 	    = L"UNKNOWN_HANDLE: Path doesn't exist",
    [HANDLE_NOT_READY] 	    = L"HANDLE_NOT_READY",
    [CORRUPT_FILE] 		    = L"CORRUPT_FILE",
    [NO_HANDLES_AVAILABLE] 	= L"NO_HANDLES_AVAILABLE: Path doesn't resolve to a valid file",
    [NO_PERMISSION] 	    = L"NO_PERMISSION:  File doesn't exist",
    [ILLEGAL_PATH] 		    = L"ILLEGAL_PATH",
    [FILE_EXITS] 	    	= L"FILE_EXITS",
    [END_OF_FILE] 	    	= L"END_OF_FILE",
    [SIZE_ERROR] 	    	= L"SIZE_ERROR: Can't write here. Is SD Card properly inserted?",
    [UNKNOWN_ERROR]     	= L"UNKNOWN_ERROR: No such directory",
    [ILLEGAL_FILENAME]  	= L"ILLEGAL_FILENAME",
    [ILLEGAL_CONNECTION]    = L"ILLEGAL_CONNECTION",
    [ERRORS_END]            = NULL,
}
#endif
;

#endif

