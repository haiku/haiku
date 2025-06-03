/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <unistd.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
dup(int fd)
{
	RETURN_AND_SET_ERRNO(_kern_dup(fd));
}


int
dup2(int oldFD, int newFD)
{
	RETURN_AND_SET_ERRNO(_kern_dup2(oldFD, newFD, 0));
}


int
dup3(int oldFD, int newFD, int flags)
{
	if (oldFD == newFD)
		RETURN_AND_SET_ERRNO(EINVAL);
	if ((flags & ~(O_CLOEXEC | O_CLOFORK)) != 0)
		RETURN_AND_SET_ERRNO(EINVAL);
	RETURN_AND_SET_ERRNO(_kern_dup2(oldFD, newFD, flags));
}
