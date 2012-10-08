/*
 * Copyright 2002-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _DEVFS_H
#define _DEVFS_H


#include <Drivers.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t devfs_unpublish_file_device(const char* path);
status_t devfs_publish_file_device(const char* path, const char* filePath);

status_t devfs_unpublish_partition(const char* path);
status_t devfs_publish_partition(const char* name, const partition_info* info);
status_t devfs_rename_partition(const char* devicePath, const char* oldName,
	const char* newName);

status_t devfs_unpublish_device(const char* path, bool disconnect);
status_t devfs_publish_device(const char* path, device_hooks* calls);
status_t devfs_publish_directory(const char* path);
status_t devfs_rescan_driver(const char* driverName);

void devfs_compute_geometry_size(device_geometry* geometry, uint64 blockCount,
	uint32 blockSize);

#ifdef __cplusplus
}
#endif

#endif /* _DEVFS_H */
