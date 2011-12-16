/*
 * Copyright 2004, François Revol.
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <FindDirectory.h>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <StorageDefs.h>


#ifndef HAIKU_BUILD_GENERATED_DIRECTORY
#	error HAIKU_BUILD_GENERATED_DIRECTORY not defined!
#endif


/*! make dir and its parents if needed */
static int
create_path(const char *path, mode_t mode)
{
	char buffer[B_PATH_NAME_LENGTH + 1];
	int pathLength;
	int i = 0;

	if (path == NULL || ((pathLength = strlen(path)) > B_PATH_NAME_LENGTH))
		return EINVAL;

	while (++i < pathLength) {
		const char *slash = strchr(&path[i], '/');
		struct stat st;

		if (slash == NULL)
			i = pathLength;
		else if (i != slash - path)
			i = slash - path;
		else
			continue;

		strlcpy(buffer, path, i + 1);
		if (stat(buffer, &st) < 0) {
			errno = 0;
			if (mkdir(buffer, mode) < 0)
				return errno;
		}
	}

	return 0;
}


status_t
find_directory(directory_which which, dev_t device, bool createIt,
	char *returnedPath, int32 pathLength)
{
	// we support only the handful of paths we need
	const char* path;
	switch (which) {
		case B_COMMON_TEMP_DIRECTORY:
			path = HAIKU_BUILD_GENERATED_DIRECTORY "/tmp";
			break;
		case B_COMMON_SETTINGS_DIRECTORY:
			path = HAIKU_BUILD_GENERATED_DIRECTORY "/common/settings";
			break;
		case B_COMMON_CACHE_DIRECTORY:
			path = HAIKU_BUILD_GENERATED_DIRECTORY "/common/cache";
			break;
		case B_USER_SETTINGS_DIRECTORY:
			path = HAIKU_BUILD_GENERATED_DIRECTORY "/user/settings";
			break;
		case B_USER_CACHE_DIRECTORY:
			path = HAIKU_BUILD_GENERATED_DIRECTORY "/user/cache";
			break;
		default:
			return B_BAD_VALUE;
	}

	// create, if necessary
	status_t error = B_OK;
	struct stat st;
	if (createIt && stat(path, &st) < 0)
		error = create_path(path, 0755);

	if (error == B_OK)
		strlcpy(returnedPath, path, pathLength);

	return error;
}

