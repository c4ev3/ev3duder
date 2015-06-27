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
struct hid_context;
union handle {int fd; hid_context * hid;};
EXTERN int (*ev3_write)(union handle *, const u8*, size_t);
EXTERN int (*ev3_read_timeout)(union handle *, u8*, size_t, int milliseconds);
EXTERN const wchar_t* (*ev3_error)(union handle *);
EXTERN union handle(*ev3_close)(union handle*);
EXTERN union handle handle;
#endif

