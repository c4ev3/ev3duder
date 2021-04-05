/**
 * @file bt-win.c
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief Windows bluetooth I/O using virtual COM port
 */
#include <stdlib.h>
#include <errno.h>

#include <windows.h>

#include "defs.h"

#define BT "COM1"

/**
 * \param [in] virtual COM port or NULL
 * \return HANDLE Windows HANDLE to virtual COM port for use with bt_{read,write,close,error}
 * \brief opens COM Porte described by device. `NULL` leads to default action
 */
void *bt_open(const char *device, const char *unused)
{
	(void) unused;
	HANDLE handle = CreateFileA(device ?: BT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (handle == INVALID_HANDLE_VALUE)
		return NULL;

	COMMTIMEOUTS timeouts = {0};
	if (!GetCommTimeouts(handle, &timeouts))
	{
		CloseHandle(handle);
		return NULL;
	}

	timeouts.ReadIntervalTimeout = 100;

	if (!SetCommTimeouts(handle, &timeouts))
	{
		CloseHandle(handle);
		return NULL;
	}

	return handle;
}

/**
 * \param [in] handle handle returned by bt_open
 * \param [in] buf byte string to write, the first byte is omitted
 * \param [in] count number of characters to be written (including leading ignored byte)
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the first byte is omitted for compatiblity with the leading report byte demanded by \p hid_write. Wrapping HIDAPI could fix this.
 */
int bt_write(void *handle, const u8 *buf, size_t count)
{
	DWORD dwBytesWritten;

	buf++;
	count--; // omit HID report number
	if (!WriteFile(handle, buf, count, &dwBytesWritten, NULL))
		return -1;

	return dwBytesWritten;
}

/**
 * \param [in] device handle returned by bt_open
 * \param [in] buf buffer to write to 
 * \param [in] count number of characters to be read
 * \param [in] milliseconds is ignored
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug milliseconds is ignored
 */
int bt_read(void *handle, u8 *buf, size_t count, int milliseconds)
{
	(void) milliseconds; // https://msdn.microsoft.com/en-us/library/windows/desktop/aa363190%28v=vs.85%29.aspx

	// TODO: read size header first, then read remaining bytes (Unix code does this)

	DWORD dwBytesRead;
	if (!ReadFile(handle, buf, count, &dwBytesRead, NULL))
		return -1;
	return dwBytesRead;
}

/**
 * \param [in] handle handle returned by bt_open
 * \brief calls CloseHandle on argument
 * \bug It takes whopping 3 seconds for the COM port to be reclaimable. If opened before that, the device is unusable for a minute.
 */
void bt_close(void *handle)
{
	CloseHandle(handle);
}

/**
 * \param [in] device handle returned by bt_open
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 */
const wchar_t *bt_error(void *fd_)
{
	(void) fd_;
	wchar_t **buf = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL, GetLastError(), 0, (wchar_t *) &buf, 0, NULL); //leaks

	return buf ? *buf : L"Error in printing error";
}
