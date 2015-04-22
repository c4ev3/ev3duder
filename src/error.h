#include "defs.h"
enum ERR {ERR_UNK = 0, ERR_ARG, ERR_IO, ERR_HID, ERR_VM, ERR_SYS, ERR_END};
enum {VM_OK = 0x03, VM_ERROR = 0x05};

struct error { enum ERR category; const char *msg; size_t reply_len; const void *reply;};

