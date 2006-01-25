/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int 
chdir(const char *path)
{
	int status = _kern_setcwd(-1, path);

	RETURN_AND_SET_ERRNO(status);
}


int 
fchdir(int fd)
{
	int status = _kern_setcwd(fd, NULL);

	RETURN_AND_SET_ERRNO(status);
}


char *
getcwd(char *buffer, size_t size)
{
	bool allocated = false;
	status_t status;

	if (buffer == NULL) {
		buffer = malloc(size = PATH_MAX);
		if (buffer == NULL) {
			errno = B_NO_MEMORY;
			return NULL;
		}

		allocated = true;
	}

	status = _kern_getcwd(buffer, size);
	if (status < B_OK) {
		if (allocated)
			free(buffer);

		errno = status;
		return NULL;
	}
	return buffer;
}


int
rmdir(const char *path)
{
	int status = _kern_remove_dir(-1, path);

	RETURN_AND_SET_ERRNO(status);
}

