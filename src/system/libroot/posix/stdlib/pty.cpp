/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <SupportDefs.h>

#include <tty.h>


int
posix_openpt(int openFlags)
{
	return open("/dev/ptmx", openFlags);
}


int
grantpt(int masterFD)
{
	return ioctl(masterFD, B_IOCTL_GRANT_TTY);
}


int
ptsname_r(int masterFD, char* name, size_t namesize)
{
	int32 index;
	if (ioctl(masterFD, B_IOCTL_GET_TTY_INDEX, &index, sizeof(index)) < 0)
		return errno;

	if (name == NULL)
		return EINVAL;

	char letter = 'p';
	int length = snprintf(name, namesize, "/dev/tt/%c%" B_PRIx32, char(letter + index / 16),
		index % 16);
	return (length + 1) > (int)namesize ? ERANGE : 0;
}


char*
ptsname(int masterFD)
{
	static char buffer[32];
	errno = ptsname_r(masterFD, buffer, sizeof(buffer));
	if (errno != 0 && errno != ERANGE)
		return NULL;
	return buffer;
}


int
unlockpt(int masterFD)
{
	// Nothing to do ATM.
	return 0;
}

