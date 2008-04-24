/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <pty.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>


int
openpty(int* _master, int* _slave, char* name, struct termios* termAttrs,
	struct winsize* windowSize)
{
	int master = posix_openpt(O_RDWR);
	if (master < 0)
    	return -1;

	int slave;
	const char *ttyName;
	if (grantpt(master) != 0 || unlockpt(master) != 0
		|| (ttyName = ptsname(master)) == NULL
		|| (slave = open(ttyName, O_RDWR | O_NOCTTY)) < 0) {
		close(master);
		return -1;
	}

	if (termAttrs != NULL && tcsetattr(master, TCSANOW, termAttrs) != 0
		|| windowSize != NULL
			&& ioctl(master, TIOCSWINSZ, windowSize, sizeof(winsize)) != 0) {
		close(slave);
		close(master);
	}

	*_master = master;
	*_slave = slave;

	if (name != NULL)
		strcpy(name, ttyName);

	return 0;
}
