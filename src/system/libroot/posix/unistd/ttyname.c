/*
 * Copyright 2003, Daniel Reinhold, danielre@users.sf.net. All rights reserved.
 * Copyright 2007, François Revol, mmu_man@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>
#include <Drivers.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <errno_private.h>


/**
 * give the name of a tty fd. threadsafe.
 * @param fd the tty to get the name from.
 * @param buffer where to store the name to.
 * @param bufferSize length of the buffer.
 * @return 0 on success, -1 on error, sets errno.
 */
int
ttyname_r(int fd, char *buffer, size_t bufferSize)
{
	struct stat fdStat;

	// first, some sanity checks:
	if (fstat(fd, &fdStat) < 0)
		return ENOTTY;

	if (!S_ISCHR(fdStat.st_mode) || !isatty(fd))
		return ENOTTY;

	// just ask devfs
	if (ioctl(fd, B_GET_PATH_FOR_DEVICE, buffer, bufferSize) < 0)
		return errno;
	return 0;
}


/**
 * give the name of a tty fd.
 * @param fd the tty to get the name from.
 * @return the name of the tty or NULL on error.
 */
char *
ttyname(int fd)
{
	static char pathname[MAXPATHLEN];

	int err = ttyname_r(fd, pathname, sizeof(pathname));
	if (err != 0) {
		__set_errno(err);
		return NULL;
	}
	return pathname;
}
