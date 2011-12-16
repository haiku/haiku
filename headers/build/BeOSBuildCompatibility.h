#ifndef BEOS_BUILD_COMPATIBILITY_H
#define BEOS_BUILD_COMPATIBILITY_H

#if defined(HAIKU_HOST_PLATFORM_CYGWIN)
#	ifndef __addr_t_defined
#		define __addr_t_defined
#	endif
#endif

#if defined(HAIKU_HOST_PLATFORM_CYGWIN) || defined(HAIKU_HOST_PLATFORM_SUNOS)
#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#endif

#ifndef S_IUMSK
#define	S_IUMSK 07777
#endif

#include <ctype.h>
#endif

#ifdef HAIKU_HOST_PLATFORM_SUNOS
#	include <limits.h>
#	ifndef NAME_MAX
#		define NAME_MAX	MAXNAMELEN
#	endif
#endif

typedef unsigned long	haiku_build_addr_t;
#define addr_t			haiku_build_addr_t

#include <Errors.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Is kernel-only under Linux.
extern size_t   strlcpy(char* dest, const char* source, size_t length);
extern size_t	strlcat(char* dest, const char* source, size_t length);

#if defined(HAIKU_HOST_PLATFORM_FREEBSD) || defined(HAIKU_HOST_PLATFORM_DARWIN)
extern size_t	strnlen(const char* string, size_t length);
#endif

#if defined(HAIKU_HOST_PLATFORM_CYGWIN) || defined(HAIKU_HOST_PLATFORM_SUNOS)
extern char*	stpcpy(char* dest, const char* src);
extern char*	strcasestr(const char* s, const char* find);
#endif

// BeOS only
extern ssize_t  read_pos(int fd, off_t pos, void* buffer, size_t count);
extern ssize_t  write_pos(int fd, off_t pos, const void* buffer, size_t count);
extern ssize_t	readv_pos(int fd, off_t pos, const struct iovec* vec,
					size_t count);
extern ssize_t	writev_pos(int fd, off_t pos, const struct iovec* vec,
					size_t count);


// There's no O_NOTRAVERSE under Linux and FreeBSD -- we replace it with a flag
// that won't be used by our tools, preferrably a non-portable one; a fixed
// constant could always lead to trouble on the host.
// We can abuse this flag for our purposes as we filter it in libroot.
#ifndef O_NOTRAVERSE
#	ifdef O_NOCTTY
#		define O_NOTRAVERSE	O_NOCTTY
#	elif defined(O_RANDOM)
#		define O_NOTRAVERSE O_RANDOM
#	else
#		error "Search for a proper replacement value for O_NOTRAVERSE"
#	endif
#endif

#ifndef S_IUMSK
#	define S_IUMSK ALLPERMS
#endif


// remap strerror()
extern char* _haiku_build_strerror(int errnum);

#ifndef BUILDING_HAIKU_ERROR_MAPPER

#undef strerror
#define strerror(errnum)	_haiku_build_strerror(errnum)

#endif	// BUILDING_HAIKU_ERROR_MAPPER


// remap file descriptor functions
int		_haiku_build_fchmod(int fd, mode_t mode);
int		_haiku_build_fchmodat(int fd, const char* path, mode_t mode, int flag);
int		_haiku_build_fstat(int fd, struct stat* st);
int		_haiku_build_fstatat(int fd, const char* path, struct stat* st,
			int flag);
int		_haiku_build_mkdirat(int fd, const char* path, mode_t mode);
int		_haiku_build_mkfifoat(int fd, const char* path, mode_t mode);
int		_haiku_build_utimensat(int fd, const char* path,
			const struct timespec times[2], int flag);
int		_haiku_build_futimens(int fd, const struct timespec times[2]);
int		_haiku_build_faccessat(int fd, const char* path, int accessMode,
			int flag);
int		_haiku_build_fchdir(int fd);
int		_haiku_build_close(int fd);
int		_haiku_build_dup(int fd);
int		_haiku_build_dup2(int fd1, int fd2);
int		_haiku_build_linkat(int toFD, const char* toPath, int pathFD,
			const char* path, int flag);
int		_haiku_build_unlinkat(int fd, const char* path, int flag);
ssize_t	_haiku_build_readlinkat(int fd, const char* path, char* buffer,
			size_t bufferSize);
int		_haiku_build_symlinkat(const char* toPath, int fd,
			const char* symlinkPath);
int		_haiku_build_ftruncate(int fd, off_t newSize);
int		_haiku_build_fchown(int fd, uid_t owner, gid_t group);
int		_haiku_build_fchownat(int fd, const char* path, uid_t owner,
			gid_t group, int flag);
int		_haiku_build_mknodat(int fd, const char* name, mode_t mode, dev_t dev);
int		_haiku_build_creat(const char* path, mode_t mode);
#ifndef _HAIKU_BUILD_DONT_REMAP_FD_FUNCTIONS
int		_haiku_build_open(const char* path, int openMode, ...);
int		_haiku_build_openat(int fd, const char* path, int openMode, ...);
int		_haiku_build_fcntl(int fd, int op, ...);
#else
int		_haiku_build_open(const char* path, int openMode, mode_t permissions);
int		_haiku_build_openat(int fd, const char* path, int openMode,
			mode_t permissions);
int		_haiku_build_fcntl(int fd, int op, int argument);
#endif
int		_haiku_build_renameat(int fromFD, const char* from, int toFD,
			const char* to);

#ifndef _HAIKU_BUILD_DONT_REMAP_FD_FUNCTIONS
#	define fchmod(fd, mode)				_haiku_build_fchmod(fd, mode)
#	define fchmodat(fd, path, mode, flag) \
		_haiku_build_fchmodat(fd, path, mode, flag)
#	define fstat(fd, st)				_haiku_build_fstat(fd, st)
#	define fstatat(fd, path, st, flag)	_haiku_build_fstatat(fd, path, st, flag)
#	define mkdirat(fd, path, mode)		_haiku_build_mkdirat(fd, path, mode)
#	define mkfifoat(fd, path, mode)		_haiku_build_mkfifoat(fd, path, mode)
#	define utimensat(fd, path, times, flag) \
		_haiku_build_utimensat(fd, path, times, flag)
#	define futimens(fd, times)			_haiku_build_futimens(fd, times)
#	define faccessat(fd, path, accessMode, flag) \
		_haiku_build_faccessat(fd, path, accessMode, flag)
#	define fchdir(fd)					_haiku_build_fchdir(fd)
#	define close(fd)					_haiku_build_close(fd)
#	define dup(fd)						_haiku_build_dup(fd)
#	define dup2(fd1, fd2)				_haiku_build_dup2(fd1, fd2)
#	define linkat(toFD, toPath, pathFD, path, flag) \
		_haiku_build_linkat(toFD, toPath, pathFD, path, flag)
#	define unlinkat(fd, path, flag)		_haiku_build_unlinkat(fd, path, flag)
#	define readlinkat(fd, path, buffer, bufferSize) \
		_haiku_build_readlinkat(fd, path, buffer, bufferSize)
#	define symlinkat(toPath, fd, symlinkPath) \
		_haiku_build_symlinkat(toPath, fd, symlinkPath)
#	define ftruncate(fd, newSize)		_haiku_build_ftruncate(fd, newSize)
#	define fchown(fd, owner, group)		_haiku_build_fchown(fd, owner, group)
#	define fchownat(fd, path, owner, group, flag) \
		_haiku_build_fchownat(fd, path, owner, group, flag)
#	define mknodat(fd, name, mode, dev) \
		_haiku_build_mknodat(fd, name, mode, dev)
#	define creat(path, mode)			_haiku_build_creat(path, mode)
#	define open(path, openMode...)		_haiku_build_open(path, openMode)
#	define openat(fd, path, openMode...) \
		_haiku_build_openat(fd, path, openMode)
#	define fcntl(fd, op...)				_haiku_build_fcntl(fd, op)
#	define renameat(fromFD, from, toFD, to) \
		_haiku_build_renameat(fromFD, from, toFD, to)
#endif	// _HAIKU_BUILD_DONT_REMAP_FD_FUNCTIONS


#ifdef __cplusplus
} // extern "C"
#endif

#endif	// BEOS_BUILD_COMPATIBILITY_H

