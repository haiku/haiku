/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <pthread.h>

#include <symbol_versioning.h>

#include <syscalls.h>

#include <signal_private.h>


int
__sigwait_beos(const sigset_t_beos* beosSet, int* _signal)
{
	// convert the given signal set and call the current version
	sigset_t set = from_beos_sigset(*beosSet);
	int error = __sigwait(&set, _signal);
	if (error != 0)
		return error;

	// translate SIGBUS to SIGSEGV
	if (*_signal == SIGBUS)
		*_signal = SIGSEGV;

	return 0;
}


int
__sigwait(const sigset_t* set, int* _signal)
{
	siginfo_t info;
	status_t error = _kern_sigwait(set, &info, 0, 0);

	pthread_testcancel();

	if (error != B_OK)
		return error;

	*_signal = info.si_signo;
	return 0;
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigwait_beos", "sigwait@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigwait", "sigwait@@", "1_ALPHA4");
