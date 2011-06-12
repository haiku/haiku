/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Daniel Reinhold, danielre@users.sf.net
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <signal.h>

#include <errno.h>
#include <stdio.h>

#include <symbol_versioning.h>

#include <signal_private.h>


static __sighandler_t
signal_common(int signal, __sighandler_t signalHandler, int flags)
{
	struct sigaction newAction, oldAction;

	// setup the replacement sigaction
	newAction.sa_handler = signalHandler;
	newAction.sa_mask = 0;
	newAction.sa_flags = flags;

	if (__sigaction(signal, &newAction, &oldAction) != 0)
		return SIG_ERR;

	// success, return the original handler
	return oldAction.sa_handler;
}


__sighandler_t
__signal_beos(int signal, __sighandler_t signalHandler)
{
	// check signal range
	if (signal < 0 || signal > MAX_SIGNAL_NUMBER_BEOS) {
		errno = EINVAL;
		return SIG_ERR;
	}

	// set the signal handler
	__sighandler_t result = signal_common(signal, signalHandler,
		SA_BEOS_COMPATIBLE_HANDLER);
	if (result == SIG_ERR)
		return SIG_ERR;

	// If the signal is SIGSEGV, set the same signal handler for SIGBUS. Those
	// constants had the same value under BeOS.
	if (signal == SIGSEGV)
		signal_common(SIGBUS, signalHandler, SA_BEOS_COMPATIBLE_HANDLER);

	return result;
}


__sighandler_t
__signal(int signal, __sighandler_t signalHandler)
{
	return signal_common(signal, signalHandler, 0);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__signal_beos", "signal@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__signal", "signal@@", "1_ALPHA4");
