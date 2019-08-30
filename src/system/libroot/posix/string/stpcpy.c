/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <sys/types.h>
#include <string.h>


/* This is not a POSIX function, but since BeOS exports it, we need to, as well. */

char *
stpcpy(char *dest, char const *src)
{
	while ((*dest++ = *src++) != '\0')
		;

	return dest - 1;
}

