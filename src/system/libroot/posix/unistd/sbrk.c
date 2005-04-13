/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <unistd.h>
#include <errno.h>


void *
sbrk(long increment)
{
	// this function is no longer supported
	return NULL;
}
