/**
 * @file btserial.h
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Input/Output wrappers for Bluetooth
 */

#include "defs.h"

/**
 * \param [in] file_in string describing the bluetooth device
 * \param [in] only for *nix: non-NULL for cases where read/write files differ
 * \return handle a opaque handle for use with bt_{read,write,close,error}
 * \brief open a bluetooth device described by device. `NULL` leads to default action
 * \see implementation at bt-win.c and bt-unix.c
 */
void *bt_open(const char *file_in, const char *file_out);

/**
 * \param [in] device handle returned by bt_open
 * \param [in] buf byte string to write, the first byte is omitted
 * \param [in] count number of characters to be written (including leading ignored byte)
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the first byte is omitted for compatiblity with the leading report byte demanded by \p hid_write. Wrapping HIDAPI could fix this.
 * \see implementation at bt-win.c and bt-unix.c
 */
int bt_write(void *device, const u8 *buf, size_t count);

/**
 * \param [in] device handle returned by bt_open
 * \param [in] buf buffer to write to 
 * \param [in] count number of characters to be read
 * \param [in] milliseconds number of milliseconds to wait at maximum. -1 is indefinitely
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the milliseconds part needs to be tested more throughly
 * \see implementation at bt-win.c and bt-unix.c
 */
int bt_read(void *device, u8 *buf, size_t count, int milliseconds);

/**
 * \param [in] device handle returned by bt_open
 * \brief Closes the resource opened by \p bt_open
 * \see implementation at bt-win.c and bt-unix.c
 */
void bt_close(void *);

/**
 * \param [in] device handle returned by bt_open
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \see implementation at bt-win.c and bt-unix.c
 * \bug it's useless
 */
const wchar_t *bt_error(void *device);
