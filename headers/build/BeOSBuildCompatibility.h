#ifndef BEOS_BUILD_COMPATIBILITY_H
#define BEOS_BUILD_COMPATIBILITY_H

typedef unsigned long haiku_build_addr_t;
#define addr_t haiku_build_addr_t

#include <Errors.h>
#include <sys/types.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Is kernel-only under Linux.
extern size_t   strlcpy(char *dest, const char *source, size_t length);
extern size_t	strlcat(char *dest, const char *source, size_t length);

// BeOS only
extern ssize_t  read_pos(int fd, off_t pos, void *buffer, size_t count);
extern ssize_t  write_pos(int fd, off_t pos, const void *buffer,size_t count);


// There's no O_NOTRAVERSE under Linux, but there's a O_NOFOLLOW, which
// means something different (open() fails when the file is a symlink), but
// we can abuse this flag for our purposes (we filter it in libroot).
#ifndef O_NOTRAVERSE
        #ifdef O_NOFOLLOW
                #define O_NOTRAVERSE O_NOFOLLOW
        #else
                #define O_NOTRAVERSE 0
        #endif
#endif

#ifndef S_IUMSK
	#define S_IUMSK ALLPERMS
#endif


// remap strerror()
extern char *_haiku_build_strerror(int errnum);

#ifndef BUILDING_HAIKU_ERROR_MAPPER

#undef strerror
#define strerror(errnum)	_haiku_build_strerror(errnum)

#endif


#ifdef __cplusplus
} // extern "C"
#endif

#endif	// BEOS_BUILD_COMPATIBILITY_H

