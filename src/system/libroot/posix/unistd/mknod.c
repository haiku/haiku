/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <unistd.h>

#include <errno_private.h>


int
mknod(const char *name, mode_t mode, dev_t dev)
{
	__set_errno(ENOTSUP);
	return -1;
}


int
mknodat(int fd, const char *name, mode_t mode, dev_t dev)
{
	__set_errno(ENOTSUP);
	return -1;
}

