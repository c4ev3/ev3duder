/**
 * @file io.c - Input/Output wrappers for Bluetooth (Win32)
 * @author Ahmad Fatoum
 * @license Code available under terms of the GNU General Public License
 */
#include <stdlib.h>
#include <errno.h>

#include <windows.h>

#include "defs.h"

#define BT L"COM1"

// ^ TODO: add multiple COM ports
void *bt_open(const wchar_t *device)
{
	HANDLE handle = CreateFileW(device ?: BT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	return handle != INVALID_HANDLE_VALUE ? handle : NULL;
}

int bt_write(void* handle, const u8* buf, size_t count)
{
	DWORD dwBytesWritten;
	
	buf++;count--; // omit HID report number
	if (!WriteFile(handle, buf, count, &dwBytesWritten, NULL))
		return -1;
	
	return dwBytesWritten;
}
int bt_read(void *handle, u8* buf, size_t count, int milliseconds)
{
	(void) milliseconds; // https://msdn.microsoft.com/en-us/library/windows/desktop/aa363190%28v=vs.85%29.aspx
	DWORD dwBytesRead;
	if (!ReadFile(handle, buf, count, &dwBytesRead, NULL))
		return -1;
	return dwBytesRead;
}
void bt_close(void *handle)
{
	CloseHandle(handle);
}
const wchar_t *bt_error(void* fd_) { (void)fd_; return L"Errors not implemented yet";}

