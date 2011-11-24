/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>

#include <errno.h>
#include <sys/stat.h>

#include <errno_private.h>
#include <syscalls.h>


char*
realpath(const char* path, char* resolved)
{
	status_t status = _kern_normalize_path(path, true, resolved);
	if (status != B_OK) {
		__set_errno(status);
		return NULL;
	}

	// The path must actually exist, not just its parent directories
	struct stat stat;
	if (lstat(resolved, &stat) != 0)
		return NULL;

	return resolved;
}
