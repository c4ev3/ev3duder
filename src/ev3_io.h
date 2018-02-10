#ifndef EV3DUDER_EV3_IO_H
#define EV3DUDER_EV3_IO_H
//! For avoiding the need to separately define and declare stuff
#undef EXTERN
#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include "defs.h"
// simple wrapper
EXTERN int (*ev3_write)(void *, const u8 *, size_t);

EXTERN int (*ev3_read_timeout)(void *, u8 *, size_t, int milliseconds);

EXTERN const wchar_t *(*ev3_error)(void *);

EXTERN void (*ev3_close)(void *);

EXTERN void *handle;
#endif

