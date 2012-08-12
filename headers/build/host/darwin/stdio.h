#ifndef _HAIKU_BUILD_COMPATIBILITY_DARWIN_STDIO
#define _HAIKU_BUILD_COMPATIBILITY_DARWIN_STDIO

#include_next <stdio.h>

__BEGIN_DECLS

int renameat(int oldFD, const char* oldPath, int newFD, const char* newPath);
int symlinkat(const char *oldPath, int fd, const char *newPath);

__END_DECLS

#endif	// _HAIKU_BUILD_COMPATIBILITY_DARWIN_STDIO
