/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <errno_private.h>


int
mknod(const char *path, mode_t mode, dev_t dev)
{
	return mknodat(AT_FDCWD, path, mode, dev);
}


int
mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
	if (dev == 0) {
		mode_t type = mode & S_IFMT;
		mode &= ~S_IFMT;
		if (type == S_IFIFO)
			return mkfifoat(fd, path, mode);
	}

	__set_errno(ENOTSUP);
	return -1;
}
