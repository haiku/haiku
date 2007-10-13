/* 
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Copyright 2001, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <unistd.h>
#include <syscalls.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


ssize_t
write(int fd, void const *buffer, size_t bufferSize)
{
	int status = _kern_write(fd, -1, buffer, bufferSize);

	RETURN_AND_SET_ERRNO(status);
}


ssize_t
write_pos(int fd, off_t pos, const void *buffer, size_t bufferSize)
{
	ssize_t status;
	if (pos < 0) {
		errno = B_BAD_VALUE;
		return -1;
	}
	status = _kern_write(fd, pos, buffer, bufferSize);

	RETURN_AND_SET_ERRNO(status);
}


ssize_t
pwrite(int fd, const void *buffer, size_t bufferSize, off_t pos)
{
	ssize_t status;
	if (pos < 0) {
		errno = B_BAD_VALUE;
		return -1;
	}
	status = _kern_write(fd, pos, buffer, bufferSize);

	RETURN_AND_SET_ERRNO(status);
}
