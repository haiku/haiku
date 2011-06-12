/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <errno.h>

#include <syscall_utils.h>

#include <symbol_versioning.h>
#include <syscalls.h>

#include <signal_private.h>


int
__sigprocmask_beos(int how, const sigset_t_beos* beosSet,
	sigset_t_beos* beosOldSet)
{
	RETURN_AND_SET_ERRNO(__pthread_sigmask_beos(how, beosSet, beosOldSet));
}


int
__pthread_sigmask_beos(int how, const sigset_t_beos* beosSet,
	sigset_t_beos* beosOldSet)
{
	// convert new signal set
	sigset_t set;
	if (beosSet != NULL)
		set = from_beos_sigset(*beosSet);

	// set the mask
	sigset_t oldSet;
	status_t error = _kern_set_signal_mask(how, beosSet != NULL ? &set : NULL,
		beosOldSet != NULL ? &oldSet : NULL);
	if (error != B_OK)
		return error;

	// convert old signal set back
	if (beosOldSet != NULL)
		*beosOldSet = to_beos_sigset(oldSet);

	return 0;
}


int
__sigprocmask(int how, const sigset_t* set, sigset_t* oldSet)
{
	RETURN_AND_SET_ERRNO(_kern_set_signal_mask(how, set, oldSet));
}


int
__pthread_sigmask(int how, const sigset_t* set, sigset_t* oldSet)
{
	return _kern_set_signal_mask(how, set, oldSet);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigprocmask_beos",
	"sigprocmask@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__pthread_sigmask_beos",
	"pthread_sigmask@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigprocmask", "sigprocmask@@",
	"1_ALPHA4");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__pthread_sigmask", "pthread_sigmask@@",
	"1_ALPHA4");
