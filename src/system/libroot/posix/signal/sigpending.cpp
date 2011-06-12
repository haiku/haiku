/*
 * Copyright 2005-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author(s):
 *		Jérôme Duval
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <signal.h>

#include <errno.h>

#include <syscall_utils.h>

#include <symbol_versioning.h>
#include <syscalls.h>

#include <signal_private.h>


int
__sigpending_beos(sigset_t_beos* beosSet)
{
	sigset_t set;
	if (__sigpending(&set) != 0)
		return -1;

	*beosSet = to_beos_sigset(set);
	return 0;
}


int
__sigpending(sigset_t* set)
{
	RETURN_AND_SET_ERRNO(_kern_sigpending(set));
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigpending_beos", "sigpending@",
	"BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigpending", "sigpending@@",
	"1_ALPHA4");
