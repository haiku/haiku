/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Daniel Reinhold, danielre@users.sf.net. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include <errno_private.h>


/*! get the attributes of the TTY device at fd */
int
tcgetattr(int fd, struct termios *termios)
{
	return ioctl(fd, TCGETA, termios);
}


/*! set the attributes for the TTY device at fd */
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
			__set_errno(EINVAL);
			return -1;
	}

	return ioctl(fd, method, termios);
}


/*! wait for all output to be transmitted */
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


/*! suspend or restart transmission */
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
			__set_errno(EINVAL);
			return -1;
	}

	return ioctl(fd, TCXONC, action);
}


/*! flush all pending data (input or output) */
int
tcflush(int fd, int queueSelector)
{
	return ioctl(fd, TCFLSH, queueSelector);
}


/*! send zero bits for the specified duration */
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
	if ((termios->c_cflag & CBAUD) == CBAUD)
		return termios->c_ispeed;

	return termios->c_cflag & CBAUD;
}


int
cfsetispeed(struct termios *termios, speed_t speed)
{
	/*	Check for custom baudrates, which must be stored in the c_ispeed
	field instead of inlined in the flags.
	Note that errors from hardware device (unsupported baudrates, etc) are
	detected only when the tcsetattr() function is called */
	if (speed > B31250) {
		termios->c_cflag |= CBAUD;
		termios->c_ispeed = speed;
		return 0;
	}

	termios->c_cflag &= ~CBAUD;
	termios->c_cflag |= speed;
	return 0;
}


speed_t
cfgetospeed(const struct termios *termios)
{
	if ((termios->c_cflag & CBAUD) == CBAUD)
		return termios->c_ospeed;

	return termios->c_cflag & CBAUD;
}


int
cfsetospeed(struct termios *termios, speed_t speed)
{
	/* Check for custom speed values (see above) */
	if (speed > B31250) {
		termios->c_cflag |= CBAUD;
		termios->c_ospeed = speed;
		return 0;
	}

	termios->c_cflag &= ~CBAUD;
	termios->c_cflag |= speed;
	return 0;
}


void
cfmakeraw(struct termios *termios)
{
	termios->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR
		| ICRNL | IXON);
	termios->c_oflag &= ~OPOST;
	termios->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termios->c_cflag &= ~(CSIZE | PARENB);
	termios->c_cflag |= CS8;
}


pid_t
tcgetsid(int fd)
{
	int sid;

	if (ioctl(fd, TIOCGSID, &sid) == 0)
		return sid;

	return -1;
}


int
tcsetsid(int fd, pid_t pid)
{
	if (pid != getsid(0)) {
		errno = EINVAL;
		return -1;
	}

	return ioctl(fd, TIOCSCTTY, NULL);
}

