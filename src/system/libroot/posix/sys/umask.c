/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <sys/stat.h>
#include <errno.h>

#include <errno_private.h>
#include <syscalls.h>
#include <umask.h>


mode_t __gUmask = 022;
	// this must be made available to open() and friends


mode_t
umask(mode_t newMask)
{
	mode_t oldMask = __gUmask;
	__gUmask = newMask;

	return oldMask;
}

