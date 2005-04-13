/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <sys/stat.h>
#include <syscalls.h>
#include <errno.h>


mode_t __gUmask = 022;
	// this must be made available to open() and friends


mode_t
umask(mode_t newMask)
{
	mode_t oldMask = __gUmask;
	__gUmask = newMask;

	return oldMask;
}

