#include <wchar.h>
#include "error.h"

const wchar_t * const ev3_error[] =
{
    [SUCCESS] 		        = L"SUCCESS",
    [UNKNOWN_HANDLE] 	    = L"UNKNOWN_HANDLE",
    [HANDLE_NOT_READY] 	    = L"HANDLE_NOT_READY",
    [CORRUPT_FILE] 		    = L"CORRUPT_FILE",
    [NO_HANDLES_AVAILABLE] 	= L"NO_HANDLES_AVAILABLE\tPath doesn't resolve to a valid file",
    [NO_PERMISSION] 	    = L"NO_PERMISSION",
    [ILLEGAL_PATH] 		    = L"ILLEGAL_PATH",
    [FILE_EXITS] 	    	= L"FILE_EXITS",
    [END_OF_FILE] 	    	= L"END_OF_FILE",
    [SIZE_ERROR] 	    	= L"SIZE_ERROR\tCan't write here. Is SD Card properly inserted?",
    [UNKNOWN_ERROR]     	= L"UNKNOWN_ERROR\tNo such directory",
    [ILLEGAL_FILENAME]  	= L"ILLEGAL_FILENAME",
    [ILLEGAL_CONNECTION]    = L"ILLEGAL_CONNECTION",
    [ERRORS_END]            = NULL,
};

