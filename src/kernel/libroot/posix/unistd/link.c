/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
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
	int status = _kern_read_link(-1, path, buffer, bufferSize);

	RETURN_AND_SET_ERRNO(status);
}


int
symlink(const char *path, const char *toPath)
{
	int status = _kern_create_symlink(-1, path, toPath, 0);

	RETURN_AND_SET_ERRNO(status);
}


int
unlink(const char *path)
{
	int status = _kern_unlink(-1, path);
	
	RETURN_AND_SET_ERRNO(status);
}


int
link(const char *path, const char *toPath)
{
	int status = _kern_create_link(path, toPath);

	RETURN_AND_SET_ERRNO(status);
}

