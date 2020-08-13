/**
 * @file error.h
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Error enumerations and decriptions
 */

#ifndef EV3DUDER_ERROR_H
#define EV3DUDER_ERROR_H


#undef EXTERN
//! For avoiding the need to separately define and declare stuff
#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include "defs.h"

//! Errors returnable from \p main
enum ERR
{
	ERR_UNK = 0, ERR_ARG, ERR_IO, ERR_FTOOBIG, ERR_NOMEM, ERR_COMM, ERR_VM, ERR_SYS, ERR_END
};

//! Statuses from VM
enum
{
	VM_DIRECT_OK = 0x02, VM_OK = 0x03, VM_DIRECT_ERROR = 0x04, VM_ERROR = 0x05
};

//! global variable for last error message
EXTERN const char *errmsg;

/**
 * Errors returned from VM
 * \see https://github.com/mindboards/ev3sources/blob/master/lms2012/c_com/source/c_com.h 
 */
enum
{
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

/**
 * \p ev3_error description strings, found by trial and error
 * \see https://github.com/mindboards/ev3sources/blob/master/lms2012/c_com/source/c_com.h 
 */
EXTERN const char *const ev3_error_msgs[ERRORS_END + 1]
#ifdef MAIN
= {
	[SUCCESS] 		        = "SUCCESS",
	[UNKNOWN_HANDLE] 	    = "UNKNOWN_HANDLE: Path doesn't exist",
	[HANDLE_NOT_READY] 	    = "HANDLE_NOT_READY",
	[CORRUPT_FILE] 		    = "CORRUPT_FILE",
	[NO_HANDLES_AVAILABLE] 	= "NO_HANDLES_AVAILABLE: Path doesn't resolve to a valid file",
	[NO_PERMISSION] 	    = "NO_PERMISSION:  File doesn't exist",
	[ILLEGAL_PATH] 		    = "ILLEGAL_PATH",
	[FILE_EXITS] 	    	= "FILE_EXITS",
	[END_OF_FILE] 	    	= "END_OF_FILE",
	[SIZE_ERROR] 	    	= "SIZE_ERROR: Can't write here. Is SD Card properly inserted?",
	[UNKNOWN_ERROR]     	= "UNKNOWN_ERROR: No such directory",
	[ILLEGAL_FILENAME]  	= "ILLEGAL_FILENAME",
	[ILLEGAL_CONNECTION]    = "ILLEGAL_CONNECTION",
	[ERRORS_END]            = NULL,
}
#endif
;

#endif
