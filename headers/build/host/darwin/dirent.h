#ifndef _HAIKU_BUILD_COMPATIBILITY_DARWIN_DIRENT
#define _HAIKU_BUILD_COMPATIBILITY_DARWIN_DIRENT

#ifdef __cplusplus
extern "C" {
#endif

#include_next <dirent.h>

DIR* fdopendir(int fd);	

#ifdef __cplusplus
}
#endif

#endif	// _HAIKU_BUILD_COMPATIBILITY_DARWIN_DIRENT
