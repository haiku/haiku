#include <syscalls.h>
#include <signal.h>

/*
 *  Copyright (c) 2002, OpenBeOS Project. All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  sigset.c:
 *  implements two sigset functions:
 *    sigemptyset() and sigfillset()
 *
 *  the other three sigset functions
 *    sigaddset(), sigdelset(), and sigismember()
 *  are inlined in the <signal.h> header file
 *  (I have no idea why there is a distinction between
 *   some being inlined and others not)
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */


int
sigemptyset(sigset_t *set)	
{
	*set = (sigset_t) 0L;
	return 0;
}


int
sigfillset(sigset_t *set)
{
	*set = (sigset_t) ~(0UL);
	return 0;
}

