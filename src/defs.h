#ifndef EV3DUDER_DEFS_H
#define EV3DUDER_DEFS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// shorter type names
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array)[0])

#define die(msg) \
	do {\
		fprintf(stderr, "[%s:%d]: %ls (%s)", __func__, __LINE__, hid_error(handle), msg);\
		exit(__LINE__);\
	} while (0)

#ifndef _GNU_SOURCE
//! \brief returns dst+len instead of len for easiert chaining
static inline void *mempcpy(void * restrict dst, const void * restrict src, size_t len) {
    return (char*)memcpy(dst, src, len) + len;
}
#endif

#define print_bytes(buf, len) \
	do {\
		u8 *ptr = (u8*)buf;\
		for (int i = 0; i != len; i++, ptr++) fprintf(stderr, "%02x ", *ptr);\
	} while (0*putc('\n', stderr))

#define print_chars(buf, len) \
	do {\
		char *ptr = buf;\
		for (int i = 0; i != len; i++, ptr++) fprintf(stderr, "%c ", *ptr);\
	} while (0*putc('\n', stderr))

#define TIMEOUT 2000 /* in milliseconds */

#endif

