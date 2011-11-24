/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <syscalls.h>

#include <errno.h>
#include <unistd.h>

#include <errno_private.h>


int
chroot(const char *path)
{
	status_t error = _kern_change_root(path);
	if (error != B_OK) {
		__set_errno(error);
		return -1;
	}

	return 0;
}
