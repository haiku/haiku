/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <sys/stat.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


// BeOS compatibility
#define BEOS_STAT_SIZE 60


int
stat(const char *path, struct stat *stat)
{
	int status = _kern_read_stat(-1, path, true, stat, BEOS_STAT_SIZE/*sizeof(struct stat)*/);

	RETURN_AND_SET_ERRNO(status);
}


int
fstat(int fd, struct stat *stat)
{
	int status = _kern_read_stat(fd, NULL, false, stat, BEOS_STAT_SIZE/*sizeof(struct stat)*/);

	RETURN_AND_SET_ERRNO(status);
}


int
lstat(const char *path, struct stat *stat)
{
	int status = _kern_read_stat(-1, path, false, stat, BEOS_STAT_SIZE/*sizeof(struct stat)*/);

	RETURN_AND_SET_ERRNO(status);
}


//	#pragma mark - BeOS compatibility


#ifndef _KERNEL_MODE

int __be_stat(const char *path, struct stat *stat);
int __be_fstat(int fd, struct stat *stat);
int __be_lstat(const char *path, struct stat *stat);


int
__be_stat(const char *path, struct stat *stat)
{
	int status = _kern_read_stat(-1, path, true, stat, BEOS_STAT_SIZE);

	RETURN_AND_SET_ERRNO(status);
}


int
__be_fstat(int fd, struct stat *stat)
{
	int status = _kern_read_stat(fd, NULL, false, stat, BEOS_STAT_SIZE);

	RETURN_AND_SET_ERRNO(status);
}


int
__be_lstat(const char *path, struct stat *stat)
{
	int status = _kern_read_stat(-1, path, false, stat, BEOS_STAT_SIZE);

	RETURN_AND_SET_ERRNO(status);
}

#endif	// !_KERNEL_MODE
