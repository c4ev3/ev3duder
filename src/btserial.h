/**
 * @file io.c - Input/Output wrappers for Bluetooth
 * @authot Ahmad Fatoum
 * @license (c) Ahmad Fatoum. Code available under terms of the GNU General Public License
 */

#include "defs.h"
void *bt_open(void);
int bt_write(void* fd, const u8* buf, size_t count);
int bt_read(void* fd, u8* buf, size_t count, int milliseconds);
const wchar_t *bt_error(void* fd); 

