#ifndef EV3DUDER_UTF8_H
#define EV3DUDER_UTF8_H

#ifdef _WIN32
#include <windows.h>

#define tchar wchar_t
#define main wmain
#define tstrcmp(dst, src) wcscmp(dst, src)
#define tfopen(path, mode) _wfopen(path, L##mode)
#define T(str) L##str
#define tstrchr(str, chr) wcschr(str, L##chr)
#define tstrlen(str) wcslen(str)
#define tgetenv(str) _wgetenv(str)
#define PRIts "ls"
#define U8(wstr, len) ({\
  int u8sz = WideCharToMultiByte(CP_UTF8, 0, wstr, len, NULL, 0, NULL, NULL); \
  char *u8str = malloc(u8sz +1); \
  u8str[u8sz] = '\0'; \
  WideCharToMultiByte(CP_UTF8, 0, wstr, len, u8str, u8sz, NULL, NULL); \
  u8str; })
#else

#define tchar char
#define tstrcmp(dst, src) strcmp(dst, src)
#define tfopen(path, mode) fopen(path, mode)
#define T(str) str
#define tstrchr(str, chr) strchr(str, chr)
#define tstrlen(str) strlen(str)
#define tgetenv(str) getenv(str)
#define PRIts "s"
#define U8(path, len) path 
#endif

#endif

