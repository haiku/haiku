/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
mkfifo(const char *path, mode_t mode)
{
	return mkfifoat(AT_FDCWD, path, mode);
}


int
mkfifoat(int fd, const char *path, mode_t mode)
{
	RETURN_AND_SET_ERRNO(_kern_create_fifo(fd, path, mode));
}
