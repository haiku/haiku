/* 
** Copyright 2003, Daniel Reinhold, danielre@users.sf.net. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <stdio.h>
#include <string.h>
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


/* tcdrain - wait for all output to be transmitted */
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


/* tcflow - suspend or restart transmission */
int
tcflow(int fd, int action)
{
	/* action will be one of the following:
	 *   TCIOFF -  input off (stops input)
	 *   TCION  -  input on  (restart input)
	 *   TCOOFF - output off (stops output)
	 *   TCOON  - output on  (restart output)
	 */
	return ioctl(fd, TCXONC, action);
}


/* tcflush - flush all pending data (input or output) */
int
tcflush(int fd, int queue_selector)
{
	return ioctl(fd, TCFLSH, queue_selector);
}


/* tcsendbreak - send zero bits for the specified duration */
int
tcsendbreak(int fd, int duration)
{	
	if (duration == 0)
		// Posix spec says this should take ~ 0.25 to 0.5 seconds
		return ioctl(fd, TCSBRK, 0);
	
	/* Posix does not specify how long the transmission time
	 * should last if 'duration' is non-zero -- i.e. it is up
	 * to each implementation to decide what to do...
	 *
	 * For simplicity, here is it assumed that a positive duration
	 * of N will mean "N time intervals" where each interval lasts
	 * from 0.25 to 0.5 seconds. A negative duration will be treated
	 * as an error.
	 */
	if (duration < 0) {
		errno = EINVAL;
		return -1;
	}
	
	while (duration-- > 0)
		if (ioctl(fd, TCSBRK, 0) < 0)
			return -1;
	
	return 0;
}



/*
 * The following four "speed control" functions are not supported
 * in the BeOS. But the interface is still part of the Posix
 * terminal IO specification, so they are implemented here as
 * stub functions for basic compliance.
 */

speed_t
cfgetispeed(const struct termios *term)
{
	return (speed_t)0;
}

int
cfsetispeed( struct termios *term, speed_t speed)
{
	return 0;
}

speed_t
cfgetospeed(const struct termios *term)
{
	return (speed_t)0;
}

int
cfsetospeed( struct termios *term, speed_t speed)
{
	return 0;
}

