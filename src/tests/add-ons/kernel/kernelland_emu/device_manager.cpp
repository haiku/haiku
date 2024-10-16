/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <stdio.h>

#include <fs/devfs.h>


extern "C" status_t
devfs_unpublish_file_device(const char* path)
{
	printf("ubpublish file device: path \"%s\"\n", path);
	return B_OK;
}


extern "C" status_t
devfs_publish_file_device(const char* path, const char* filePath)
{
	printf("publish file device: path \"%s\" (file path \"%s\")\n", path, filePath);
	return B_OK;
}


extern "C" status_t
devfs_unpublish_partition(const char *path)
{
	printf("unpublish partition: %s\n", path);
	return B_OK;
}


extern "C" status_t
devfs_publish_partition(const char *path, const partition_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	printf("publish partition: %s (device \"%s\", size %lld)\n", path, info->device, info->size);
	return B_OK;
}


extern "C" status_t
devfs_rename_partition(const char* devicePath, const char* oldName, const char* newName)
{
	printf("rename partition: %s (oldName \"%s\", newName \"%s\")\n",
		devicePath, oldName, newName);
	return B_OK;
}
