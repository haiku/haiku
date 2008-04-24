/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DRIVERS_TTY_H
#define _DRIVERS_TTY_H

#include <termios.h>

#define B_IOCTL_GET_TTY_INDEX	(TCGETA + 32)	/* param is int32* */
#define B_IOCTL_GRANT_TTY		(TCGETA + 33)	/* no param (cf. grantpt()) */


#endif	// _DRIVERS_TTY_H
