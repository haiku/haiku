#ifndef HAIKU_BUILD_COMPATIBILITY_H
#define HAIKU_BUILD_COMPATIBILITY_H

#include <sys/types.h>

#define _BE_ERRNO_H_
	// this is what Dano/Zeta is using
#include <Errors.h>
#include <string.h>

// no addr_t under standard BeOS
typedef unsigned long haiku_build_addr_t;
#define addr_t haiku_build_addr_t

#ifndef DEFFILEMODE
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#endif

#ifndef FS_WRITE_FSINFO_NAME
#define	FS_WRITE_FSINFO_NAME	0x0001
#endif

#ifndef B_CURRENT_TEAM
#define B_CURRENT_TEAM	0


#endif


#ifdef __cplusplus
extern "C" {
#endif

extern size_t	strnlen(const char *string, size_t count);

extern size_t	strlcat(char *dest, const char *source, size_t length);
extern size_t   strlcpy(char *dest, const char *source, size_t length);

extern char		*strcasestr(const char *string, const char *searchString);

extern float	roundf(float value);

#ifdef __cplusplus
} // extern "C"
#endif

// These are R1-specific extensions
#ifndef HAIKU_TARGET_PLATFORM_HAIKU

#define B_TRANSLATION_MAKE_VERSION(major,minor,revision) ((major << 8) | ((minor << 4) & 0xf0) | (revision & 0x0f))
#define B_TRANSLATION_MAJOR_VERSION(v) (v >> 8)
#define B_TRANSLATION_MINOR_VERSION(v) ((v >> 4) & 0xf)
#define B_TRANSLATION_REVISION_VERSION(v) (v & 0xf)

#define B_BAD_DATA (B_NOT_ALLOWED+1)

#endif

#endif	// HAIKU_BUILD_COMPATIBILITY_H

