/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <sys/stat.h>

#include <errno_private.h>
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
