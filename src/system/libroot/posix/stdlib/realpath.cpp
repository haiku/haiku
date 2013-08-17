/*
 * Copyright 2013, Olivier Coursière, olivier.coursiere@laposte.net.
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
	char* resolvedPath = resolved;

	if (resolvedPath == NULL) {
		resolvedPath = (char*)malloc(PATH_MAX + 1);
		if (resolvedPath == NULL) {
			__set_errno(B_NO_MEMORY);
			return NULL;
		}
	}

	status_t status = _kern_normalize_path(path, true, resolvedPath);
	if (status != B_OK) {
		__set_errno(status);
		if (resolved == NULL)
			free(resolvedPath);
		return NULL;
	}

	// The path must actually exist, not just its parent directories
	struct stat stat;
	if (lstat(resolvedPath, &stat) != 0) {
		if (resolved == NULL)
			free(resolvedPath);
		return NULL;
	}

	return resolvedPath;
}
