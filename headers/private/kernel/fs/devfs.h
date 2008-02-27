/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _DEVFS_H
#define _DEVFS_H


#include <Drivers.h>

struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

void devfs_add_preloaded_drivers(struct kernel_args* args);

status_t devfs_add_driver(const char *path);

void devfs_driver_added(const char *path);
void devfs_driver_removed(const char *path);

status_t devfs_unpublish_file_device(const char *path);
status_t devfs_publish_file_device(const char *path, const char *filePath);

status_t devfs_unpublish_partition(const char *path);
status_t devfs_publish_partition(const char *path, const partition_info *info);

status_t devfs_unpublish_device(const char *path, bool disconnect);
status_t devfs_publish_device(const char *path, device_hooks *calls);
status_t devfs_publish_directory(const char *path);
status_t devfs_rescan_driver(const char *driverName);

#ifdef __cplusplus
}
#endif

#endif /* _DEVFS_H */
