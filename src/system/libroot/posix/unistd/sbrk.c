/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>
#include <errno.h>

#include <errno_private.h>


/* in hoard wrapper */
extern void *(*sbrk_hook)(long);


void *
sbrk(long increment)
{
	// TODO: we only support extending the heap for now using this method
	if (increment <= 0)
		return NULL;

	if (sbrk_hook)
		return (*sbrk_hook)(increment);

	return NULL;
}
