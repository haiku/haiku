/* 
** Copyright 2003, Daniel Reinhold, danielre@users.sf.net. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <errno.h>
#include <unistd.h>
#include <termios.h>


/*
 * tcsetattr - set the attributes for the tty device at fd
 *             (using the structure in the last arg for the values)
 */
int
tcsetattr(int fd, int opt, const struct termios *tp)
{
	switch (opt) {
		case TCSANOW:
			// set the attributes immediately
			return ioctl(fd, TCSETA, tp);
		
		case TCSADRAIN:
			// wait for ouput to finish before setting the attributes
			return ioctl(fd, TCSETAW, tp);
		
		case TCSAFLUSH:
			// wait for ouput to finish and then flush the input buffer
			// before setting the attributes
			return ioctl(fd, TCSETAF, tp);
		
		default:
			// no other valid options
			errno = EINVAL;
			return -1;
	}
}

