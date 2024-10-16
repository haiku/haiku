/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include <vfs.h>


extern "C" status_t
vfs_stat_node_ref(dev_t device, ino_t inode, struct stat *stat)
{
	return B_NOT_SUPPORTED;
}


extern "C" status_t
vfs_normalize_path(const char* path, char* buffer, size_t bufferSize,
	bool traverseLink, bool kernel)
{
	return B_NOT_SUPPORTED;
}


extern "C" status_t
vfs_entry_ref_to_path(dev_t device, ino_t inode, const char *leaf,
	bool kernel, char *path, size_t pathLength)
{
	return B_NOT_SUPPORTED;
}


extern "C" status_t
vfs_unmount(dev_t mountID, uint32 flags)
{
	return B_NOT_SUPPORTED;
}
