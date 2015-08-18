/**
 * @file tcp.h
 * @author Ahmad Fatoum
 * @copyright (c) 2015 Ahmad Fatoum. Code available under terms of the GNU General Public License 3.0
 * @brief BSD sockets Input/Output wrappers
 */

#include "defs.h"

//FIXME: encapsulate
struct tcp_handle {
#ifdef _WIN32
	void* sock; // HANDLE/SOCKET
#else
	int fd[2];
#endif
	char serial[15];
	char ip[48]; // enough for ipv6
	unsigned tcp_port;
	char name[96];
	char protocol[8];
};
//FIXME: copy more stuff here
/**
 * \param [in] serial of Ev3 to connect to
 * \return handle a opaque handle for use with tcp_{read,write,close,error}
 * \brief open a bluetooth device described by device. `NULL` leads to default action
 * \see implementation at bt-win.c and bt-unix.c
 */ 
void *tcp_open(const char *serial);

/**
 * \param [in] device handle returned by bt_open
 * \param [in] buf byte string to write, the first byte is omitted
 * \param [in] count number of characters to be written (including leading ignored byte)
 * \return status -1 on error. bytes read otherwise.	
 * \brief writes buf[1] till buf[count - 2] to device
 * \bug the first byte is omitted for compatiblity with the leading report byte demanded by \p hid_write. Wrapping HIDAPI could fix this.
 * \see implementation at bt-win.c and bt-unix.c
 */
#ifdef _WIN32
int tcp_write(void* device, const u8* buf, size_t count);
#else
extern int (*tcp_write)(void* device, const u8* buf, size_t count);
#endif
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
#ifdef _WIN32
extern int tcp_read(void* device, u8* buf, size_t count, int milliseconds);
#else
extern int (*tcp_read)(void* device, u8* buf, size_t count, int milliseconds);
#endif

/**
 * \param [in] device handle returned by bt_open
 * \brief Closes the resource opened by \p bt_open
 * \see implementation at bt-win.c and bt-unix.c
 */
void tcp_close(void*);
/**
 * \param [in] device handle returned by bt_open
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \see implementation at bt-win.c and bt-unix.c
 * \bug it's useless
 */
const wchar_t *tcp_error(void* device); 

/**
 * \param [in] device handle returned by bt_open
 * \return message An error string
 * \brief Returns an error string describing the last error occured
 * \see implementation at bt-win.c and bt-unix.c
 * \bug it's useless
 */
const wchar_t *tcp_info(void* device); 

