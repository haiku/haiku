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


const char * const
sys_siglist[NSIG] = {
	/*  0              */  "Signal 0",
	/*  1 - SIGHUP     */  "Hangup",
	/*  2 - SIGINT     */  "Interrupt",
	/*  3 - SIGQUIT    */  "Quit",
	/*  4 - SIGILL     */  "Illegal instruction",
	/*  5 - SIGCHLD    */  "Child exited",
	/*  6 - SIGABRT    */  "Abort",
	/*  7 - SIGPIPE    */  "Broken pipe",
	/*  8 - SIGFPE     */  "Floating point exception",
	/*  9 - SIGKILL    */  "Killed (by death)",
	/* 10 - SIGSTOP    */  "Stopped",
	/* 11 - SIGSEGV    */  "Segmentation violation",
	/* 12 - SIGCONT    */  "Continued",
	/* 13 - SIGTSTP    */  "Stopped (tty output)",
	/* 14 - SIGALRM    */  "Alarm",
	/* 15 - SIGTERM    */  "Termination requested",
	/* 16 - SIGTTIN    */  "Stopped (tty input)",
	/* 17 - SIGTTOU    */  "Stopped (tty output)",
	/* 18 - SIGUSR1    */  "User defined signal 1",
	/* 19 - SIGUSR2    */  "User defined signal 2",
	/* 20 - SIGWINCH   */  "Window size changed",
	/* 21 - SIGKILLTHR */  "Kill Thread",
	/* 22 - SIGTRAP    */  "Trace/breakpoint trap",
	/* 23 - SIGPOLL    */  "Pollable event",
	/* 24 - SIGPROF    */  "Profiling timer expired",
	/* 25 - SIGSYS     */  "Bad system call", 
	/* 26 - SIGURG     */  "High bandwidth data is available at socket",
	/* 27 - SIGVTALRM  */  "Virtual timer expired",
	/* 28 - SIGXCPU    */  "CPU time limit exceeded",
	/* 29 - SIGXFSZ    */  "File size limit exceeded",
};


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

