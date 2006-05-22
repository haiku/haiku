/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <unistd.h>
#include <errno.h>

/* in hoard wrapper */
extern void *(*sbrk_hook)(long);

void *
sbrk(long increment)
{
	if (sbrk_hook)
		return (*sbrk_hook)(increment);
	return NULL;
}
