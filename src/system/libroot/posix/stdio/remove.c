/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <errno.h>

#include <errno_private.h>
#include <syscalls.h>


int
remove(const char* path)
{
	// TODO: find a better way that does not require two syscalls for directories
	int status = _kern_unlink(-1, path);
	if (status == B_IS_A_DIRECTORY)
		status = _kern_remove_dir(-1, path);

	if (status != B_OK) {
		__set_errno(status);
		return -1;
	}

	return status;
}

