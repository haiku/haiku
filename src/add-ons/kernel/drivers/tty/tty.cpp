/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "tty_private.h"


#define TTY_TRACE
#ifdef TTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


int32
get_tty_index(const char *name)
{
	// device names follow this form: "pt/%c%x"
	int8 digit = name[4];
	if (digit >= 'a')
		digit -= 'a';
	else
		digit -= '0';

	return (name[3] - 'p') * digit;
}

