/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _DEVFS_H
#define _DEVFS_H

#include <vfs.h>
#include <drivers.h>

int bootstrap_devfs(void);

/* api drivers will use to publish devices */
int devfs_publish_device(const char *path, void *ident, device_hooks *calls);

#endif /* _DEVFS_H */
