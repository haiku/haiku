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
#include <pthread.h>

#include <syscall_utils.h>

#include <symbol_versioning.h>
#include <syscalls.h>

#include <signal_private.h>


int
__sigsuspend_beos(const sigset_t_beos* beosMask)
{
	sigset_t mask = from_beos_sigset(*beosMask);
	return __sigsuspend(&mask);
}


int
__sigsuspend(const sigset_t* mask)
{
	errno = _kern_sigsuspend(mask);

	pthread_testcancel();

	return -1;
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigsuspend_beos", "sigsuspend@",
	"BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigsuspend", "sigsuspend@@",
	"1_ALPHA4");
