/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/stat.h>
#include <syscalls.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


// R5 compatibility

#define R5_STAT_SIZE 60
#undef stat
#undef fstat
#undef lstat

extern int stat(const char *path, struct stat *stat);
extern int fstat(int fd, struct stat *stat);
extern int lstat(const char *path, struct stat *stat);


int
stat(const char *path, struct stat *stat)
{
	return _stat(path, stat, R5_STAT_SIZE);
}


int
fstat(int fd, struct stat *stat)
{
	return _fstat(fd, stat, R5_STAT_SIZE);
}


int
lstat(const char *path, struct stat *stat)
{
	return _lstat(path, stat, R5_STAT_SIZE);
}


//	#pragma mark -


int
_stat(const char *path, struct stat *stat, size_t statSize)
{
	int status = _kern_read_path_stat(path, true, stat, statSize);

	RETURN_AND_SET_ERRNO(status);
}


int
_lstat(const char *path, struct stat *stat, size_t statSize)
{
	int status = _kern_read_path_stat(path, false, stat, statSize);

	RETURN_AND_SET_ERRNO(status);
}


int
_fstat(int fd, struct stat *stat, size_t statSize)
{
	int status = _kern_read_stat(fd, stat, statSize);

	RETURN_AND_SET_ERRNO(status);
}
