#ifndef _HAIKU_BUILD_COMPATIBILITY_DARWIN_SYS_STAT
#define _HAIKU_BUILD_COMPATIBILITY_DARWIN_SYS_STAT

#include_next <sys/stat.h>
#include <sys/cdefs.h>

#ifndef UTIME_NOW
#	define UTIME_NOW	(-1)
#	define UTIME_OMIT	(-2)

	__BEGIN_DECLS

	/* assume that futimens() and utimensat() aren't available */
	int futimens(int fd, const struct timespec times[2]);
	int utimensat(int fd, const char* path, const struct timespec times[2],
		int flag);

	__END_DECLS

#	ifndef _HAIKU_BUILD_NO_FUTIMENS
#		define _HAIKU_BUILD_NO_FUTIMENS		1
#	endif
#	ifndef _HAIKU_BUILD_NO_UTIMENSAT
#		define _HAIKU_BUILD_NO_UTIMENSAT	1
#	endif
#endif

__BEGIN_DECLS

int fchmodat(int fd, const char* path, mode_t mode, int flag);
int fstatat(int fd, const char *path, struct stat *st, int flag);
int mkdirat(int fd, const char *path, mode_t mode);
int mkfifoat(int fd, const char *path, mode_t mode);
int mknodat(int fd, const char *name, mode_t mode, dev_t dev);

__END_DECLS

#endif	/* _HAIKU_BUILD_COMPATIBILITY_DARWIN_SYS_STAT */
