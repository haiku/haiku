/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _SYSTEM_VFS_DEFS_H
#define _SYSTEM_VFS_DEFS_H

#include <sys/stat.h>

#include <SupportDefs.h>


struct fd_info {
	int		number;
	int32	open_mode;
	dev_t	device;
	ino_t	node;
};

#endif	/* _SYSTEM_VFS_DEFS_H */
