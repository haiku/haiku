/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _DEVFS_H
#define _DEVFS_H


#include <Drivers.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t devfs_unpublish_partition(const char *path);
status_t devfs_publish_partition(const char *path, const partition_info *info);

status_t devfs_publish_device(const char *path, void *ident, device_hooks *calls);

#ifdef __cplusplus
}
#endif

#endif /* _DEVFS_H */
