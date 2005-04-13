/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include <syscalls.h>


/**	isatty - is the given file descriptor bound to a terminal device?
 *	a simple call to fetch the terminal control attributes suffices
 *	(only a valid tty device will succeed)
 */

int
isatty(int fd)
{
	struct termios termios;

	return _kern_ioctl(fd, TCGETA, &termios, sizeof(struct termios)) == B_OK;
		// we don't use tcgetattr() here in order to keep errno unchanged
}


/** returns the name of the controlling terminal */

char *
ctermid(char *s)
{
	static char defaultBuffer[L_ctermid];
	char *name = ttyname(STDOUT_FILENO);
		// we assume that stdout is our controlling terminal...

	if (s == NULL)
		s = defaultBuffer;

	return strcpy(s, name ? name : "");
}


int
tcsetpgrp(int fd, pid_t pgrpid)
{
	return ioctl(fd, TIOCSPGRP, &pgrpid);
}


pid_t
tcgetpgrp(int fd)
{
	pid_t foregroundProcess;
	int status = ioctl(fd, TIOCGPGRP, &foregroundProcess);
	if (status == 0)
		return foregroundProcess;

	return -1;
}
