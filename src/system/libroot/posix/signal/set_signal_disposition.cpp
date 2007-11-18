/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>


sighandler_t
sigset(int signal, sighandler_t signalHandler)
{
	struct sigaction newAction;
	struct sigaction oldAction;

	newAction.sa_handler = signalHandler;
	newAction.sa_flags = 0;
	if (signal == SIGCHLD && signalHandler == SIG_IGN) {
		// In case of SIGCHLD the specification requires SA_NOCLDWAIT behavior.
		newAction.sa_flags |= SA_NOCLDWAIT;
	}

	sigemptyset(&newAction.sa_mask);
	if (sigaction(signal, signalHandler == SIG_HOLD ? NULL : &newAction,
			&oldAction) == -1) {
		return SIG_ERR;
	}
	
	sigset_t newSet;
	sigset_t oldSet;

	sigemptyset(&newSet);
	sigaddset(&newSet, signal);
	
	if (signalHandler == SIG_HOLD) {
		if (sigprocmask(SIG_BLOCK, &newSet, &oldSet) == -1)
			return SIG_ERR;
	} else {
		// Disposition is not equal to SIG_HOLD, so sig will be 
		// removed from the calling process' signal mask.
		if (sigprocmask(SIG_UNBLOCK, &newSet, &oldSet) == -1)
			return SIG_ERR;
	}

	if (sigismember(&oldSet, signal))
		return SIG_HOLD;
	else
		return oldAction.sa_handler;
}
