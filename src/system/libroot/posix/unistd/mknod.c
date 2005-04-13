/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>
#include <errno.h>


int
mknod(const char *name, mode_t mode, dev_t dev)
{
	errno = B_BAD_VALUE;
	return -1;
}

