/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <symbol_versioning.h>

#include <signal_private.h>


__sighandler_t
__sigset_beos(int signal, __sighandler_t signalHandler)
{
	// prepare new action
	struct sigaction_beos newAction;
	newAction.sa_handler = signalHandler;
	newAction.sa_flags = 0;
	if (signal == SIGCHLD && signalHandler == SIG_IGN) {
		// In case of SIGCHLD the specification requires SA_NOCLDWAIT behavior.
		newAction.sa_flags |= SA_NOCLDWAIT;
	}
	newAction.sa_mask = 0;

	// set action
	struct sigaction_beos oldAction;
	if (__sigaction_beos(signal,
			signalHandler == SIG_HOLD ? NULL : &newAction, &oldAction) == -1) {
		return SIG_ERR;
	}

	// prepare new signal mask
	sigset_t_beos newSet = 0;
	__sigaddset_beos(&newSet, signal);

	// set signal mask
	sigset_t_beos oldSet;
	if (signalHandler == SIG_HOLD) {
		if (__pthread_sigmask_beos(SIG_BLOCK, &newSet, &oldSet) == -1)
			return SIG_ERR;
	} else {
		// Disposition is not equal to SIG_HOLD, so sig will be removed from the
		// calling process' signal mask.
		if (__pthread_sigmask_beos(SIG_UNBLOCK, &newSet, &oldSet) == -1)
			return SIG_ERR;
	}

	return __sigismember_beos(&oldSet, signal)
		? SIG_HOLD : oldAction.sa_handler;
}


__sighandler_t
__sigset(int signal, __sighandler_t signalHandler)
{
	// prepare new action
	struct sigaction newAction;
	newAction.sa_handler = signalHandler;
	newAction.sa_flags = 0;
	if (signal == SIGCHLD && signalHandler == SIG_IGN) {
		// In case of SIGCHLD the specification requires SA_NOCLDWAIT behavior.
		newAction.sa_flags |= SA_NOCLDWAIT;
	}
	newAction.sa_mask = 0;

	// set action
	struct sigaction oldAction;
	if (__sigaction(signal,
			signalHandler == SIG_HOLD ? NULL : &newAction, &oldAction) == -1) {
		return SIG_ERR;
	}

	// prepare new signal mask
	sigset_t newSet = 0;
	sigaddset(&newSet, signal);

	// set signal mask
	sigset_t oldSet;
	if (signalHandler == SIG_HOLD) {
		if (sigprocmask(SIG_BLOCK, &newSet, &oldSet) == -1)
			return SIG_ERR;
	} else {
		// Disposition is not equal to SIG_HOLD, so sig will be
		// removed from the calling process' signal mask.
		if (sigprocmask(SIG_UNBLOCK, &newSet, &oldSet) == -1)
			return SIG_ERR;
	}

	return sigismember(&oldSet, signal) ? SIG_HOLD : oldAction.sa_handler;
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigset_beos", "sigset@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__sigset", "sigset@@", "1_ALPHA4");
