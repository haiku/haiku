/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>

#include <errno_private.h>
#include <syscall_utils.h>
#include <syscalls.h>


extern "C" int
closefrom(int lowFd)
{
	RETURN_AND_SET_ERRNO(_kern_close_range(lowFd, ~0, 0));
}


extern "C" int
close_range(u_int minFd, u_int maxFd, int flags)
{
	RETURN_AND_SET_ERRNO(_kern_close_range(minFd, maxFd, flags));
}

