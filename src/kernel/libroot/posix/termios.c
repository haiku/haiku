/* 
** Copyright 2003, Daniel Reinhold, danielre@users.sf.net. All rights reserved.
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <termios.h>
#include <unistd.h>
#include <errno.h>


/** get the attributes of the TTY device at fd */

int
tcgetattr(int fd, struct termios *termios)
{
	return ioctl(fd, TCGETA, termios);
}


/** set the attributes for the TTY device at fd */

int
tcsetattr(int fd, int opt, const struct termios *termios)
{
	int method;

	switch (opt) {
		case TCSANOW:
			// set the attributes immediately
			method = TCSETA;
			break;
		case TCSADRAIN:
			// wait for ouput to finish before setting the attributes
			method = TCSETAW;
			break;
		case TCSAFLUSH:
			method = TCSETAF;
			break;

		default:
			// no other valid options
			errno = EINVAL;
			return -1;
	}

	return ioctl(fd, method, termios);
}


/** wait for all output to be transmitted */

int
tcdrain(int fd)
{
	/* Some termios implementations have a TIOCDRAIN command
	 * expressly for this purpose (e.g. ioctl(fd, TIOCDRAIN, 0).
	 * However, the BeOS implementation adheres to another
	 * interface which uses a non-zero last parameter to the
	 * TCSBRK ioctl to signify this functionality.
	 */
	return ioctl(fd, TCSBRK, 1);
}


/** suspend or restart transmission */

int
tcflow(int fd, int action)
{
	switch (action) {
		case TCIOFF:
		case TCION:
		case TCOOFF:
		case TCOON:
			break;

		default:
			errno = EINVAL;
			return -1;
	}

	return ioctl(fd, TCXONC, action);
}


/** flush all pending data (input or output) */

int
tcflush(int fd, int queueSelector)
{
	return ioctl(fd, TCFLSH, queueSelector);
}


/** send zero bits for the specified duration */

int
tcsendbreak(int fd, int duration)
{	
	// Posix spec says this should take ~ 0.25 to 0.5 seconds.
	// As the interpretation of the duration is undefined, we'll just ignore it
	return ioctl(fd, TCSBRK, 0);
}


speed_t
cfgetispeed(const struct termios *termios)
{
	return termios->c_ispeed;
}


int
cfsetispeed(struct termios *termios, speed_t speed)
{
	termios->c_ispeed = speed;
	return 0;
}


speed_t
cfgetospeed(const struct termios *termios)
{
	return termios->c_ospeed;
}


int
cfsetospeed(struct termios *termios, speed_t speed)
{
	termios->c_ospeed = speed;
	return 0;
}

