/*
 *  Copyright (c) 2002-2004, Haiku Project. All rights reserved.
 *  Distributed under the terms of the Haiku license.
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 */


#include <stdio.h>
#include <signal.h>
#include <errno.h>


sig_func_t
signal(int sig, sig_func_t signalHandler)
{
	struct sigaction newAction, oldAction;
	int err;

	// setup the replacement sigaction
	newAction.sa_handler = signalHandler;
	newAction.sa_mask = 0;
	newAction.sa_flags = 0;

	// install the replacement sigaction for the signal 'sig' into
	// the current thread (the original is copied into the last param)
	err = sigaction(sig, &newAction, &oldAction);
	if (err < B_OK) {
		errno = 1;
		return SIG_ERR;
	}

	// success, return the original handler
	return oldAction.sa_handler;
}

