/**
 * @file btserial.h
 * @author Ahmad Fatoum
 * @license Copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 2.0
 * @brief Input/Output wrappers for Bluetooth
 */

#include "defs.h"

/**
 * \param [in] device string describing the bluetooth device
 * \return handle a opaque handle for use with bt_{read,write,close,error}
 * \brief open a bluetooth device described by device. `NULL` leads to default action
 * \see implementation at btwin.c and btunix.c
 */ 
union handle bt_open(const char *device);

/**
 * \param [in] device handle returned by bt_open
 * \param [in] buf byte string to write, the first byte is omitted
 * \param [in] count number of characters to be written (including leading ignored byte)
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the first byte is omitted for compatiblity with the leading report byte demanded by \p hid_write. Wrapping HIDAPI could fix this.
 * \see implementation at btwin.c and btunix.c
 */ 
int bt_write(union handle device, const u8* buf, size_t count);

/**
 * \param [in] device handle returned by bt_open
 * \param [in] buf buffer to write to 
 * \param [in] count number of characters to be read
 * \param [in] milliseconds number of milliseconds to wait at maximum. -1 is indefinitely
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the milliseconds part needs to be tested more throughly
 * \see implementation at btwin.c and btunix.c
 */ 
int bt_read(union handle device, u8* buf, size_t count, int milliseconds);

/**
 * \param [in] device handle returned by bt_open
 * \brief Closes the resource opened by \p bt_open
 * \see implementation at btwin.c and btunix.c
 */
 void bt_close(union handle);
/**
 * \param [in] device handle returned by bt_open
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \see implementation at btwin.c and btunix.c
 * \bug it's useless
 */
const wchar_t *bt_error(union handle device); 

