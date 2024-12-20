/*
 * Copyright 2023, Haiku, inc.
 *
 * Distributed under the terms of the MIT License.
 */


#include <termios.h>


int
cfsetspeed(struct termios *termios, speed_t speed)
{
	/* Custom values are stored in two parts for ABI compatibility reasons.
	 * Standard values are inlined in c_cflag. */
	if (speed > B31250) {
		termios->c_cflag |= CBAUD;
		termios->c_ospeed = speed & 0xFFFF;
		termios->c_ospeed_high = speed >> 16;
		termios->c_ispeed = termios->c_ospeed;
		termios->c_ispeed_high = termios->c_ospeed_high;
		return 0;
	}

	termios->c_cflag &= ~CBAUD;
	termios->c_cflag |= speed;
	return 0;
}

