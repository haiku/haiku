/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <unistd.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <errno_private.h>
#include <syscalls.h>


ssize_t
write(int fd, void const *buffer, size_t bufferSize)
{
	int status = _kern_write(fd, -1, buffer, bufferSize);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(status);
}


ssize_t
write_pos(int fd, off_t pos, const void *buffer, size_t bufferSize)
{
	if (pos < 0)
		RETURN_AND_SET_ERRNO_TEST_CANCEL(B_BAD_VALUE);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_write(fd, pos, buffer, bufferSize));
}


ssize_t
pwrite(int fd, const void *buffer, size_t bufferSize, off_t pos)
{
	if (pos < 0)
		RETURN_AND_SET_ERRNO_TEST_CANCEL(B_BAD_VALUE);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_write(fd, pos, buffer, bufferSize));
}
