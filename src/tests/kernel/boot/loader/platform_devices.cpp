/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Handle.h"

#include <boot/platform.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>


static status_t
add_device(const char *path, struct list *list)
{
	Handle *device = new Handle(path);
	if (device == NULL)
		return B_NO_MEMORY;

	if (device->InitCheck() != B_OK) {
		delete device;
		return B_ERROR;
	}

	printf("add \"%s\" to list of boot devices\n", path);
	list_add_item(list, device);

	return B_OK;
}


static status_t
recursive_add_device(const char *path, struct list *list)
{
	DIR *dir = opendir(path);
	if (dir == NULL)
		return errno;

	struct dirent *dirent;
	while ((dirent = readdir(dir)) != NULL) {
		// we don't care about names with a leading dot (incl. "." and "..")
		if (dirent->d_name[0] == '.')
			continue;

		char nextPath[PATH_MAX];
		strcpy(nextPath, path);
		strcat(nextPath, "/");
		strcat(nextPath, dirent->d_name);

		// Note, this doesn't care about if it's a directory or not!
		if (!strcmp(dirent->d_name, "raw")
			&& add_device(nextPath, list) == B_OK)
			continue;

		recursive_add_device(nextPath, list);
	}
	closedir(dir);

	return B_OK;
}


status_t
platform_get_boot_devices(struct stage2_args *args, struct list *list)
{
	add_device("/boot/home/test-file-device", list);
	recursive_add_device("/dev/disk", list);

	return B_OK;
}

