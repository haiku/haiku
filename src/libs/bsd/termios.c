/*
 * Copyright 2023, Haiku, inc.
 *
 * Distributed under the terms of the MIT License.
 */


#include <termios.h>


int
cfsetspeed(struct termios *termios, speed_t speed)
{
	/* Custom speed values are stored in c_ispeed and c_ospeed.
	 * Standard values are inlined in c_cflag. */
	if (speed > B31250) {
		termios->c_cflag |= CBAUD;
		termios->c_ispeed = speed;
		termios->c_ospeed = speed;
		return 0;
	}

	termios->c_cflag &= ~CBAUD;
	termios->c_cflag |= speed;
	return 0;
}

