#ifndef _HAIKU_BUILD_COMPATIBILITY_DARWIN_FCNTL
#define _HAIKU_BUILD_COMPATIBILITY_DARWIN_FCNTL


#include_next <fcntl.h>
#include <sys/time.h>


/*
 * Magic value that specify the use of the current working directory
 * to determine the target of relative file paths in the openat() and
 * similar syscalls.
 */
#ifndef AT_FDCWD
#define AT_FDCWD			-100	/* CWD FD for the *at() functions */
#endif

/*
 * Miscellaneous flags for the *at() syscalls.
 */
#ifndef AT_EACCESS
#define AT_EACCESS			0x100	/* faccessat() */
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW	0x200	/* fstatat(), fchmodat(), fchownat(),
									   utimensat() */
#endif
#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW	0x400	/* linkat() */
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR		0x800	/* unlinkat() */
#endif

__BEGIN_DECLS

int unlinkat(int fd, const char *path, int flag);
int futimesat(int fd, const char *path, const struct timeval times[2]);

__END_DECLS

#endif	// _HAIKU_BUILD_COMPATIBILITY_DARWIN_FCNTL
