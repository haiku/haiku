/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <sys/stat.h>
#include <errno.h>

#include <syscalls.h>
#include <umask.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
mkdir(const char* path, mode_t mode)
{
	status_t status = _kern_create_dir(-1, path, mode & ~__gUmask);

	RETURN_AND_SET_ERRNO(status);
}
