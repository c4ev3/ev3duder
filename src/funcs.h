#ifndef EV3DUDER_FUNCS_H
#define EV3DUDER_FUNCS_H

#include <stdio.h>

extern int up(FILE*, const char*);
extern int dl(const char*, FILE*);
extern int test(void);
extern int exec(const char*);
extern int ls(const char*);
extern int rm(const char*);
extern int raw(const char*, size_t len);
extern int mkdir(const char*);

#endif

