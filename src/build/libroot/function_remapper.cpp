/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdarg.h>
#include <fcntl.h>

#include "remapped_functions.h"


#if __GNUC__ >= 4
#	define HIDDEN_FUNCTION(function)	do {} while (0)
#	define HIDDEN_FUNCTION_ATTRIBUTE	__attribute__((visibility("hidden")))
#else
#	define HIDDEN_FUNCTION(function)	asm volatile(".hidden " #function)
#	define HIDDEN_FUNCTION_ATTRIBUTE
#endif


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fchmod(int fd, mode_t mode)
{
	HIDDEN_FUNCTION(fchmod);

	return _haiku_build_fchmod(fd, mode);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fchmodat(int fd, const char* path, mode_t mode, int flag)
{
	HIDDEN_FUNCTION(fchmodat);

	return _haiku_build_fchmodat(fd, path, mode, flag);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fstat(int fd, struct stat* st)
{
	HIDDEN_FUNCTION(fstat);

	return _haiku_build_fstat(fd, st);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fstatat(int fd, const char* path, struct stat* st, int flag)
{
	HIDDEN_FUNCTION(fstatat);

	return _haiku_build_fstatat(fd, path, st, flag);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
mkdirat(int fd, const char* path, mode_t mode)
{
	HIDDEN_FUNCTION(mkdirat);

	return _haiku_build_mkdirat(fd, path, mode);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
mkfifoat(int fd, const char* path, mode_t mode)
{
	HIDDEN_FUNCTION(mkfifoat);

	return _haiku_build_mkfifoat(fd, path, mode);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
utimensat(int fd, const char* path, const struct timespec times[2], int flag)
{
	HIDDEN_FUNCTION(utimensat);

	return _haiku_build_utimensat(fd, path, times, flag);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
futimens(int fd, const struct timespec times[2])
{
	HIDDEN_FUNCTION(futimens);

	return _haiku_build_futimens(fd, times);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
faccessat(int fd, const char* path, int accessMode, int flag)
{
	HIDDEN_FUNCTION(faccessat);

	return _haiku_build_faccessat(fd, path, accessMode, flag);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fchdir(int fd)
{
	HIDDEN_FUNCTION(fchdir);

	return _haiku_build_fchdir(fd);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
close(int fd)
{
	HIDDEN_FUNCTION(close);

	return _haiku_build_close(fd);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
dup(int fd)
{
	HIDDEN_FUNCTION(dup);

	return _haiku_build_dup(fd);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
dup2(int fd1, int fd2)
{
	HIDDEN_FUNCTION(dup2);

	return _haiku_build_dup2(fd1, fd2);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
linkat(int toFD, const char* toPath, int pathFD, const char* path, int flag)
{
	HIDDEN_FUNCTION(linkat);

	return _haiku_build_linkat(toFD, toPath, pathFD, path, flag);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
unlinkat(int fd, const char* path, int flag)
{
	HIDDEN_FUNCTION(unlinkat);

	return _haiku_build_unlinkat(fd, path, flag);
}


extern "C" ssize_t HIDDEN_FUNCTION_ATTRIBUTE
readlinkat(int fd, const char* path, char* buffer, size_t bufferSize)
{
	HIDDEN_FUNCTION(readlinkat);

	return _haiku_build_readlinkat(fd, path, buffer, bufferSize);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
symlinkat(const char* toPath, int fd, const char* symlinkPath)
{
	HIDDEN_FUNCTION(symlinkat);

	return _haiku_build_symlinkat(toPath, fd, symlinkPath);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
ftruncate(int fd, off_t newSize)
{
	HIDDEN_FUNCTION(ftruncate);

	return _haiku_build_ftruncate(fd, newSize);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fchown(int fd, uid_t owner, gid_t group)
{
	HIDDEN_FUNCTION(fchown);

	return _haiku_build_fchown(fd, owner, group);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag)
{
	HIDDEN_FUNCTION(fchownat);

	return _haiku_build_fchownat(fd, path, owner, group, flag);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
mknodat(int fd, const char* name, mode_t mode, dev_t dev)
{
	HIDDEN_FUNCTION(mknodat);

	return _haiku_build_mknodat(fd, name, mode, dev);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
creat(const char* path, mode_t mode)
{
	HIDDEN_FUNCTION(creat);

	return _haiku_build_creat(path, mode);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
open(const char* path, int openMode, ...)
{
	HIDDEN_FUNCTION(open);

	mode_t permissions = 0;
	if ((openMode & O_CREAT) != 0) {
		va_list args;
		va_start(args, openMode);
		mode_t mask = umask(0);
		umask(mask);
		permissions = va_arg(args, int);
		va_end(args);
	}

	return _haiku_build_open(path, openMode, permissions);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
openat(int fd, const char* path, int openMode, ...)
{
	HIDDEN_FUNCTION(openat);

	mode_t permissions = 0;
	if ((openMode & O_CREAT) != 0) {
		va_list args;
		va_start(args, openMode);
		mode_t mask = umask(0);
		umask(mask);
		permissions = va_arg(args, int);
		va_end(args);
	}

	return _haiku_build_openat(fd, path, openMode, permissions);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fcntl(int fd, int op, ...)
{
	HIDDEN_FUNCTION(fcntl);

	va_list args;
	va_start(args, op);
	int argument = va_arg(args, int);
	va_end(args);

	return _haiku_build_fcntl(fd, op, argument);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
renameat(int fromFD, const char* from, int toFD, const char* to)
{
	HIDDEN_FUNCTION(renameat);

	return _haiku_build_renameat(fromFD, from, toFD, to);
}


#if defined(HAIKU_HOST_USE_XATTR) && defined(HAIKU_HOST_PLATFORM_HAIKU)
// fs_attr_* functions only need to be remapped on Haiku


extern "C" void* HIDDEN_FUNCTION_ATTRIBUTE
fs_open_attr_dir(const char *path)
{
	HIDDEN_FUNCTION(fs_open_attr_dir);

	return _haiku_build_fs_open_attr_dir(path);
}


extern "C" void* HIDDEN_FUNCTION_ATTRIBUTE
fs_lopen_attr_dir(const char *path)
{
	HIDDEN_FUNCTION(fs_lopen_attr_dir);

	return _haiku_build_fs_lopen_attr_dir(path);
}


extern "C" void* HIDDEN_FUNCTION_ATTRIBUTE
fs_fopen_attr_dir(int fd)
{
	HIDDEN_FUNCTION(fs_fopen_attr_dir);

	return _haiku_build_fs_fopen_attr_dir(fd);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fs_close_attr_dir(void *dir)
{
	HIDDEN_FUNCTION(fs_close_attr_dir);

	return _haiku_build_fs_close_attr_dir(dir);
}


extern "C" void* HIDDEN_FUNCTION_ATTRIBUTE
fs_read_attr_dir(void *dir)
{
	HIDDEN_FUNCTION(fs_read_attr_dir);

	return _haiku_build_fs_read_attr_dir(dir);
}


extern "C" void HIDDEN_FUNCTION_ATTRIBUTE
fs_rewind_attr_dir(void *dir)
{
	HIDDEN_FUNCTION(fs_rewind_attr_dir);

	_haiku_build_fs_rewind_attr_dir(dir);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fs_fopen_attr(int fd, const char *attribute, uint32 type, int openMode)
{
	HIDDEN_FUNCTION(fs_fopen_attr);

	return _haiku_build_fs_fopen_attr(fd, attribute, type, openMode);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fs_close_attr(int fd)
{
	HIDDEN_FUNCTION(fs_close_attr);

	return _haiku_build_fs_close_attr(fd);
}


extern "C" ssize_t HIDDEN_FUNCTION_ATTRIBUTE
fs_read_attr(int fd, const char* attribute, uint32 type, off_t pos,
	void *buffer, size_t readBytes)
{
	HIDDEN_FUNCTION(fs_read_attr);

	return _haiku_build_fs_read_attr(fd, attribute, type, pos, buffer,
		readBytes);
}


extern "C" ssize_t HIDDEN_FUNCTION_ATTRIBUTE
fs_write_attr(int fd, const char* attribute, uint32 type, off_t pos,
	const void *buffer, size_t writeBytes)
{
	HIDDEN_FUNCTION(fs_write_attr);

	return _haiku_build_fs_write_attr(fd, attribute, type, pos, buffer,
		writeBytes);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fs_remove_attr(int fd, const char* attribute)
{
	HIDDEN_FUNCTION(fs_remove_attr);

	return _haiku_build_fs_remove_attr(fd, attribute);
}


extern "C" int HIDDEN_FUNCTION_ATTRIBUTE
fs_stat_attr(int fd, const char *attribute, struct attr_info *attrInfo)
{
	HIDDEN_FUNCTION(fs_stat_attr);

	return _haiku_build_fs_stat_attr(fd, attribute, attrInfo);
}

#endif // defined(HAIKU_HOST_USE_XATTR) && defined(HAIKU_HOST_PLATFORM_HAIKU)
