#ifndef _HAIKU_BUILD_COMPATIBILITY_FREEBSD_SYS_STAT
#define _HAIKU_BUILD_COMPATIBILITY_FREEBSD_SYS_STAT

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

#endif	/* _HAIKU_BUILD_COMPATIBILITY_FREEBSD_SYS_STAT */
