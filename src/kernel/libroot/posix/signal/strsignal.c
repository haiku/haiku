#include <stdio.h>
#include <signal.h>
#include <errno.h>

/*
 *  Copyright (c) 2002, OpenBeOS Project. All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  strsignal.c:
 *  implements the signal function strsignal()
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */


const char *
strsignal(int sig)
{
	const char *s = NULL;
	
	if (sig < 0 || sig > MAX_SIGNO)
		return "Bogus signal number";
	
	if (sig < NSIG)
		s = sys_siglist[sig];
	
	if (s)
		return s;
	else {
		static char buf[40];
		sprintf(buf, "Unknown Signal %d", sig);
		return (const char *) buf;
	}
}

