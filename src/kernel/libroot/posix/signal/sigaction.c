#include <syscalls.h>
#include <signal.h>

/*
 *  Copyright (c) 2002, OpenBeOS Project. All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  sigaction.c:
 *  implements the signal function sigaction()
 *  this is merely a wrapper for a syscall
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */


int
sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	return sys_sigaction(sig, act, oact);
}

