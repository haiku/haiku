/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>
#include <errno.h>


/* in hoard wrapper */
extern void *(*sbrk_hook)(long);


void *
sbrk(long increment)
{
// TODO: disabled for now - let's GDB crash, so it obviously doesn't work yet
//	if (sbrk_hook)
//		return (*sbrk_hook)(increment);
	return NULL;
}
