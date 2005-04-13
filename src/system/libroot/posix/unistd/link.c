/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>
#include <errno.h>
#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


ssize_t
readlink(const char *path, char *buffer, size_t bufferSize)
{
	status_t status = _kern_read_link(-1, path, buffer, &bufferSize);
	if (status < B_OK && status != B_BUFFER_OVERFLOW) {
		errno = status;
		return -1;
	}

	return bufferSize;
}


int
symlink(const char *toPath, const char *symlinkPath)
{
	int status = _kern_create_symlink(-1, symlinkPath, toPath, 0);

	RETURN_AND_SET_ERRNO(status);
}


int
unlink(const char *path)
{
	int status = _kern_unlink(-1, path);
	
	RETURN_AND_SET_ERRNO(status);
}


int
link(const char *toPath, const char *linkPath)
{
	int status = _kern_create_link(linkPath, toPath);

	RETURN_AND_SET_ERRNO(status);
}

