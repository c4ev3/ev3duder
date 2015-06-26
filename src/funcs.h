/**
 * @file   funcs.h
 * @license Copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 2.0
 * @brief  contains declarations for ev3 commands
 *
 * All commands take FILE* loc arguments for loc files
 * and UTF-8 encoded rem file names, if any.
 * On POSIX, this needs no changes. On Windows, for multibyte input
 * UCS-2 to UTF-8 conversion would need to be handled by caller
 * @see branch win32-unicode for a possible way to implement \p main.
 *      All calls to these functions should be in main.c
 * @author Ahmad Fatoum
 */
#ifndef EV3DUDER_FUNCS_H
#define EV3DUDER_FUNCS_H

#include <stdio.h>
#include <stddef.h>
/**
 * @name    General Usage
 * @ingroup EV3 commands
 *
 * @param [in] loc Local FILE*s
 * @param [in] rem Remote EV3 UTF-8 encoded path strings
 *
 * @retval error according to enum #ERR
 */

//! upload local file \p loc to remote destination \p rem
extern int up(FILE* loc, const char *rem);

//! download remote source \p rem to local file \p loc
extern int dl(const char *rem, FILE* loc);

//! print HID information, beep and exit
extern int test(void);

//! run remote .rbf file \p rem via VM
extern int run(const char *rem);

//! list contents of remote directory \p rem
extern int ls(const char *rem);


//! remove remote file or directory \p rem
extern int rm(const char *rem);

//! create directory \p rem on remote system
extern int mkdir(const char *rem);

//! fill \p *buf with a rbf file executing \p cmd
extern size_t mkrbf(char **buf, const char *cmd);

#if 0 // not yet implemented
//! concatenate contents of \p count of \p rem FILEs to the EV3's LCD
extern int cat(const char *rem, size_t count);
#endif
#endif

