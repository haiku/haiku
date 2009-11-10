/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#define B_ENABLE_INCOMPLETE_POSIX_AT_SUPPORT 1
	// make the *at() functions and AT_* macros visible

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
	return readlinkat(AT_FDCWD, path, buffer, bufferSize);
}


ssize_t
readlinkat(int fd, const char *path, char *buffer, size_t bufferSize)
{
	size_t linkLen = bufferSize;
	status_t status = _kern_read_link(fd, path, buffer, &linkLen);
	if (status < B_OK) {
		errno = status;
		return -1;
	}

	// If the buffer is big enough, null-terminate the string. That's not
	// required by the standard, but helps non-conforming apps.
	if (linkLen < bufferSize)
		buffer[linkLen] = '\0';

	return linkLen;
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

