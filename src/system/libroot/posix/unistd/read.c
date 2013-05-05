/*
 * Copyright 2001, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <errno_private.h>
#include <syscalls.h>


ssize_t
read(int fd, void* buffer, size_t bufferSize)
{
	ssize_t status = _kern_read(fd, -1, buffer, bufferSize);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(status);
}


ssize_t
read_pos(int fd, off_t pos, void* buffer, size_t bufferSize)
{
	if (pos < 0)
		RETURN_AND_SET_ERRNO_TEST_CANCEL(B_BAD_VALUE);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_read(fd, pos, buffer, bufferSize));
}


ssize_t
pread(int fd, void* buffer, size_t bufferSize, off_t pos)
{
	if (pos < 0)
		RETURN_AND_SET_ERRNO_TEST_CANCEL(B_BAD_VALUE);

	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_read(fd, pos, buffer, bufferSize));
}
