/*
 * Copyright 2024 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <devctl.h>
#include <syscalls.h>


int
posix_devctl(int fd, int cmd, void* __restrict argument, size_t size, int* __restrict result)
{
	int status = _kern_ioctl(fd, cmd, argument, size);

	if (status < B_OK)
		return status;

	if (result != NULL)
		*result = status;

	return B_OK;
}
