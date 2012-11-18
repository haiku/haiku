/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdlib.h>

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


char*
ptsname(int masterFD)
{
	int32 index;
	if (ioctl(masterFD, B_IOCTL_GET_TTY_INDEX, &index, sizeof(index)) < 0)
		return NULL;

	static char buffer[32];
	
	char letter = 'p';
	snprintf(buffer, sizeof(buffer), "/dev/tt/%c%" B_PRIx32,
		char(letter + index / 16), index % 16);

	return buffer;
}


int
unlockpt(int masterFD)
{
	// Noting to do ATM.
	return 0;
}
