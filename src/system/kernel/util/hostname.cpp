/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <FindDirectory.h>
#include <StorageDefs.h>

#include <errno_private.h>


static status_t
get_path(char *path, bool create)
{
	status_t status = find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, create,
		path, B_PATH_NAME_LENGTH);
	if (status != B_OK)
		return status;

	strlcat(path, "/network", B_PATH_NAME_LENGTH);
	if (create)
		mkdir(path, 0755);
	strlcat(path, "/hostname", B_PATH_NAME_LENGTH);
	return B_OK;
}


extern "C" int
sethostname(const char *hostName, size_t nameSize)
{
	char path[B_PATH_NAME_LENGTH];
	if (get_path(path, false) != B_OK) {
		__set_errno(B_ERROR);
		return -1;
	}

	int file = open(path, O_WRONLY | O_CREAT, 0644);
	if (file < 0)
		return -1;

	nameSize = min_c(nameSize, MAXHOSTNAMELEN);

	if (write(file, hostName, nameSize) != (ssize_t)nameSize
		|| write(file, "\n", 1) != 1) {
		close(file);
		return -1;
	}

	close(file);
	return 0;
}


extern "C" int
gethostname(char *hostName, size_t nameSize)
{
	// look up hostname from network settings hostname file

	char path[B_PATH_NAME_LENGTH];
	if (get_path(path, false) != B_OK) {
		__set_errno(B_ERROR);
		return -1;
	}

	int file = open(path, O_RDONLY);
	if (file < 0)
		return -1;

	nameSize = min_c(nameSize, MAXHOSTNAMELEN);

	int length = read(file, hostName, nameSize - 1);
	close(file);

	if (length < 0)
		return -1;

	hostName[length] = '\0';

	char *end = strpbrk(hostName, "\r\n\t");
	if (end != NULL)
		end[0] = '\0';

	return 0;
}

