/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
chdir(const char *path)
{
	RETURN_AND_SET_ERRNO(_kern_setcwd(-1, path));
}


int
fchdir(int fd)
{
	RETURN_AND_SET_ERRNO(_kern_setcwd(fd, NULL));
}


char *
getcwd(char *buffer, size_t size)
{
	bool allocated = false;
	status_t status;

	if (buffer == NULL) {
		buffer = malloc(size = PATH_MAX);
		if (buffer == NULL) {
			__set_errno(B_NO_MEMORY);
			return NULL;
		}

		allocated = true;
	}

	status = _kern_getcwd(buffer, size);
	if (status < B_OK) {
		if (allocated)
			free(buffer);

		__set_errno(status);
		return NULL;
	}
	return buffer;
}


int
rmdir(const char *path)
{
	RETURN_AND_SET_ERRNO(_kern_remove_dir(-1, path));
}

