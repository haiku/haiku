/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Daniel Reinhold, danielre@users.sf.net
 *		Ingo Weinhod, ingo_weinhold@gmx.de
 */


#include <errno.h>
#include <signal.h>

#include <syscall_utils.h>

#include <signal_defs.h>
#include <symbol_versioning.h>
#include <syscalls.h>

#include <errno_private.h>
#include <signal_private.h>


int
__sigaction_beos(int signal, const struct sigaction_beos* beosAction,
	struct sigaction_beos* beosOldAction)
{
	// convert the new action
	struct sigaction action;
	if (beosAction != NULL) {
		action.sa_handler = beosAction->sa_handler;
		action.sa_mask = from_beos_sigset(beosAction->sa_mask);
		action.sa_flags = beosAction->sa_flags | SA_BEOS_COMPATIBLE_HANDLER;
		action.sa_userdata = beosAction->sa_userdata;
	}

	struct sigaction oldAction;
	if (__sigaction(signal, beosAction != NULL ? &action : NULL,
			beosOldAction != NULL ? &oldAction : NULL) != 0) {
		return -1;
	}

	// If the signal is SIGSEGV, set the same signal action for SIGBUS. Those
	// constants had the same value under BeOS.
	if (beosAction != NULL && signal == SIGSEGV)
		__sigaction(SIGBUS, &action, NULL);

	// convert the old action back
	if (beosOldAction != NULL) {
		beosOldAction->sa_handler = oldAction.sa_handler;
		beosOldAction->sa_mask = to_beos_sigset(oldAction.sa_mask);
		beosOldAction->sa_flags
			= oldAction.sa_flags & ~(int)SA_BEOS_COMPATIBLE_HANDLER;
		beosOldAction->sa_userdata = oldAction.sa_userdata;
	}

	return 0;
}


int
__sigaction(int signal, const struct sigaction* action,
	struct sigaction* oldAction)
{
	RETURN_AND_SET_ERRNO(_kern_sigaction(signal, action, oldAction));
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigaction_beos", "sigaction@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigaction", "sigaction@@", "1_ALPHA4");
