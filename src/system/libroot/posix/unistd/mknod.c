/* 
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <unistd.h>


int
mknod(const char *name, mode_t mode, dev_t dev)
{
	errno = ENOTSUP;
	return -1;
}


int
mknodat(int fd, const char *name, mode_t mode, dev_t dev)
{
	errno = ENOTSUP;
	return -1;
}

