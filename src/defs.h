#ifndef EV3DUDER_DEFS_H
#define EV3DUDER_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// shorter type names
typedef uint8_t 	u8;
typedef uint16_t 	u16;
typedef uint32_t 	u32;
typedef uint64_t 	u64;

typedef int8_t 		i8;
typedef int16_t 	i16;
typedef int32_t 	i32;
typedef int64_t 	i64;


#define TIMEOUT 2000 /* in milliseconds */


#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


#if !defined(_GNU_SOURCE) && !defined(mempcpy) && !defined(__MINGW32__)
//! \brief returns dst+len instead of len for easiert chaining
static inline void *mempcpy(void *restrict dst, const void *restrict src, size_t len)
{
	return (char *) memcpy(dst, src, len) + len;
}

#endif

#define print_bytes(buf, len) \
	do { \
		u8 *ptr = (u8*)buf; \
		for (int i = 0; i != len; i++, ptr++) \
			fprintf(stderr, "%02x ", *ptr); \
		putc('\n', stderr); \
	} while (0)

#endif
