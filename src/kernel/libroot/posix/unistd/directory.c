/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <unistd.h>
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
	int status = _kern_getcwd(buffer, size);
	if (status < B_OK) {
		errno = status;
		return NULL;
	}
	return buffer;
}


int
rmdir(const char *path)
{
	int status = _kern_remove_dir(path);

	RETURN_AND_SET_ERRNO(status);
}

