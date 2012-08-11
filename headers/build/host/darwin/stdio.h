#ifndef _HAIKU_BUILD_COMPATIBILITY_DARWIN_STDIO
#define _HAIKU_BUILD_COMPATIBILITY_DARWIN_STDIO

#ifdef __cplusplus
extern "C" {
#endif

#include_next <stdio.h>

int renameat(int oldFD, const char* oldPath, int newFD, const char* newPath);
int symlinkat(const char *oldPath, int fd, const char *newPath);

#ifdef __cplusplus
}
#endif

#endif	// _HAIKU_BUILD_COMPATIBILITY_DARWIN_STDIO
