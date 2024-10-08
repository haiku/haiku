/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
rename(const char *from, const char *to)
{
	return renameat(AT_FDCWD, from, AT_FDCWD, to);
}


int
renameat(int fromFD, const char* from, int toFD, const char* to)
{
	RETURN_AND_SET_ERRNO(_kern_rename(fromFD, from, toFD, to));
}
