/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
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


// we're using some libroot specials
extern int __libc_argc;
extern char **__libc_argv;
extern const char *__progname;

extern bool gShowMenu;


static status_t
get_device(const char *path, Node **_device)
{
	Handle *device = new Handle(path);
	if (device == NULL)
		return B_NO_MEMORY;

	if (device->InitCheck() != B_OK) {
		fprintf(stderr, "%s: Could not open image \"%s\": %s\n",
			__progname, path, strerror(device->InitCheck()));

		delete device;
		return B_ERROR;
	}

	*_device = device;
	return B_OK;
}


static status_t
add_device(const char *path, NodeList *list)
{
	Node *device;
	status_t status = get_device(path, &device);
	if (status < B_OK)
		return status;

	printf("add \"%s\" to list of boot devices\n", path);
	list->Add(device);

	return B_OK;
}


static status_t
recursive_add_device(const char *path, NodeList *list)
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


static char *
get_next_argument(int32 *cookie, bool options)
{
	int32 i = *cookie + 1;

	if (i == 1 && !options) {
		// filter out options at the start
		while (i < __libc_argc && __libc_argv[i][0] == '-')
			i++;
	}

	for (; i < __libc_argc; i++) {
		// Options come at the start
		if (options && __libc_argv[i][0] != '-')
			return NULL;

		*cookie = i;
		return __libc_argv[i];
	}

	return NULL;
}


static char *
get_next_option(int32 *cookie)
{
	return get_next_argument(cookie, true);
}


static char *
get_next_device(int32 *cookie)
{
	return get_next_argument(cookie, false);
}


//	#pragma mark -


status_t
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	// we accept a boot device from the command line
	status_t status = B_ERROR;
	Node *device;

	int32 cookie = 0;
	char *path = get_next_device(&cookie);
	if (path != NULL)
		status = get_device(path, &device);
	else
		status = get_device("/boot/home/test-file-device", &device);

	if (status == B_OK)
		devicesList->Add(device);

	return status;
}


status_t
platform_get_boot_partitions(struct stage2_args* args, Node* bootDevice,
	NodeList *list, NodeList *partitionList)
{
	NodeIterator iterator = list->GetIterator();
	boot::Partition *partition = NULL;
	while ((partition = (boot::Partition *)iterator.Next()) != NULL) {
		// ToDo: just take the first partition for now
		partitionList->Insert(partition);
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(struct stage2_args *args, NodeList *list)
{
	int32 cookie = 0;
	if (get_next_device(&cookie) != NULL) {
		// add the devices provided on the command line
		char *path;
		while ((path = get_next_device(&cookie)) != NULL)
			add_device(path, list);
	}

	bool addDevices = true;
	bool scsi = true;
	char *option;
	cookie = 0;
	while ((option = get_next_option(&cookie)) != NULL) {
		if (!strcmp(option, "--no-devices"))
			addDevices = false;
		else if (!strcmp(option, "--no-scsi"))
			scsi = false;
		else if (!strcmp(option, "--menu"))
			gShowMenu = true;
		else {
			fprintf(stderr, "usage: %s [OPTIONS] [image ...]\n"
				"  --no-devices\tDon't add real devices from /dev/disk\n"
				"  --no-scsi\tDon't add SCSI devices (might be problematic with some\n"
				"\t\tUSB mass storage drivers)\n"
				"  --menu\tShow boot menu\n", __progname);
			exit(0);
		}
	}

	if (addDevices) {
		recursive_add_device("/dev/disk/ide", list);
		recursive_add_device("/dev/disk/virtual", list);

		if (scsi)
			recursive_add_device("/dev/disk/scsi", list);
	}

	return B_OK;
}


status_t 
platform_register_boot_device(Node *device)
{
	return B_OK;
}
