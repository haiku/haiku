#include <syscalls.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

/*
 *  Copyright (c) 2002, OpenBeOS Project. All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  signal.c:
 *  implements the standard C library function signal()
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */


sig_func_t
signal(int sig, sig_func_t signal_handler)
{
	int err;
	struct sigaction repl, orig;
	
	// setup the replacement sigaction
	repl.sa_handler = signal_handler;
	repl.sa_mask = 0;
	repl.sa_flags = 0;
	
	// install the replacement sigaction for the signal 'sig' into
	// the current thread (the original is copied into the last param)
	err = sys_sigaction(sig, &repl, &orig);
	if (err == 0)
		// success, return the original handler
		return orig.sa_handler;
	else {
		errno = err;
		return SIG_ERR;
	}

}

