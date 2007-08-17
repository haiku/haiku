/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Daniel Reinhold (danielre@users.sf.net)
 */


#include <signal.h>
#include <stdio.h>


sighandler_t
signal(int sig, sighandler_t signalHandler)
{
	struct sigaction newAction, oldAction;

	// setup the replacement sigaction
	newAction.sa_handler = signalHandler;
	newAction.sa_mask = 0;
	newAction.sa_flags = 0;

	if (sigaction(sig, &newAction, &oldAction) != 0)
		return SIG_ERR;

	// success, return the original handler
	return oldAction.sa_handler;
}

