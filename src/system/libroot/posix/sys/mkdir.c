/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#define B_ENABLE_INCOMPLETE_POSIX_AT_SUPPORT 1
	// make the *at() functions and AT_* macros visible

#include <sys/stat.h>
#include <errno.h>

#include <syscalls.h>
#include <syscall_utils.h>
#include <umask.h>


int
mkdir(const char* path, mode_t mode)
{
	RETURN_AND_SET_ERRNO(_kern_create_dir(-1, path, mode & ~__gUmask));
}


int
mkdirat(int fd, const char *path, mode_t mode)
{
	RETURN_AND_SET_ERRNO(_kern_create_dir(fd, path, mode & ~__gUmask));
}
