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
#define PRIts "ls"

#define U8(wstr) ({\
  size_t len = wcslen(wstr); \
  int u8sz = WideCharToMultiByte(CP_UTF8, 0, wstr, len, NULL, 0, NULL, NULL); \
  char *u8str = malloc(len); \
  WideCharToMultiByte(CP_UTF8, 0, wstr, len, u8str, u8sz, NULL, NULL); \
  u8str; })

#else

#define tchar char
#define tstrcmp(dst, src) strcmp(dst, src)
#define tfopen(path, mode) fopen(path, mode)
#define T(str) str
#define tstrchr(str, chr) strchr(str, chr)
#define U8 
#define PRIts "s"
#endif

#endif

