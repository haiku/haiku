/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _SYSTEM_VFS_DEFS_H
#define _SYSTEM_VFS_DEFS_H


#include <limits.h>
#include <sys/stat.h>

#include <SupportDefs.h>


struct fd_info {
	int		number;
	int32	open_mode;
	dev_t	device;
	ino_t	node;
};


/* maximum write size to a pipe/FIFO that is guaranteed not to be interleaved
   with other writes (aka {PIPE_BUF}; must be >= _POSIX_PIPE_BUF) */
#define VFS_FIFO_ATOMIC_WRITE_SIZE	PIPE_BUF

/* pipe/FIFO buffer capacity */
#define VFS_FIFO_BUFFER_CAPACITY	(64 * 1024)

// make sure the constant values are sane
#if VFS_FIFO_ATOMIC_WRITE_SIZE < _POSIX_PIPE_BUF
#	error VFS_FIFO_ATOMIC_WRITE_SIZE < _POSIX_PIPE_BUF!
#endif


#endif	/* _SYSTEM_VFS_DEFS_H */
