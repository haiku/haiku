/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Daniel Reinhold (danielre@users.sf.net)
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <string.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include <signal_defs.h>


static const char* const kInvalidSignalNumberText = "Bogus signal number";


const char * const
sys_siglist[NSIG] = {
	/*  0 */				"Signal 0",
	/*  1 - SIGHUP */		"Hangup",
	/*  2 - SIGINT  */		"Interrupt",
	/*  3 - SIGQUIT */		"Quit",
	/*  4 - SIGILL */		"Illegal instruction",
	/*  5 - SIGCHLD */		"Child exited",
	/*  6 - SIGABRT */		"Abort",
	/*  7 - SIGPIPE */		"Broken pipe",
	/*  8 - SIGFPE */		"Floating point exception",
	/*  9 - SIGKILL */		"Killed (by death)",
	/* 10 - SIGSTOP */		"Stopped",
	/* 11 - SIGSEGV */		"Segmentation violation",
	/* 12 - SIGCONT */		"Continued",
	/* 13 - SIGTSTP */		"Stopped (tty output)",
	/* 14 - SIGALRM */		"Alarm",
	/* 15 - SIGTERM */		"Termination requested",
	/* 16 - SIGTTIN */		"Stopped (tty input)",
	/* 17 - SIGTTOU */		"Stopped (tty output)",
	/* 18 - SIGUSR1 */		"User defined signal 1",
	/* 19 - SIGUSR2 */		"User defined signal 2",
	/* 20 - SIGWINCH */		"Window size changed",
	/* 21 - SIGKILLTHR */	"Kill Thread",
	/* 22 - SIGTRAP */		"Trace/breakpoint trap",
	/* 23 - SIGPOLL */		"Pollable event",
	/* 24 - SIGPROF */		"Profiling timer expired",
	/* 25 - SIGSYS */		"Bad system call",
	/* 26 - SIGURG */		"High bandwidth data is available at socket",
	/* 27 - SIGVTALRM */	"Virtual timer expired",
	/* 28 - SIGXCPU */		"CPU time limit exceeded",
	/* 29 - SIGXFSZ */		"File size limit exceeded",
	/* 30 - SIGBUS */		"Bus error",
	/* 31 - SIGRESERVED1 */	"Reserved signal 1",
	/* 32 - SIGRESERVED2 */	"Reserved signal 2",
	/* 33 - realtime 1 */	"Realtime signal 1",
	/* 34 - realtime 2 */	"Realtime signal 2",
	/* 35 - realtime 3 */	"Realtime signal 3",
	/* 36 - realtime 4 */	"Realtime signal 4",
	/* 37 - realtime 5 */	"Realtime signal 5",
	/* 38 - realtime 6 */	"Realtime signal 6",
	/* 39 - realtime 7 */	"Realtime signal 7",
	/* 40 - realtime 8 */	"Realtime signal 8",
	/* 41 - invalid */		kInvalidSignalNumberText,
	/* 42 - invalid */		kInvalidSignalNumberText,
	/* 43 - invalid */		kInvalidSignalNumberText,
	/* 44 - invalid */		kInvalidSignalNumberText,
	/* 45 - invalid */		kInvalidSignalNumberText,
	/* 46 - invalid */		kInvalidSignalNumberText,
	/* 47 - invalid */		kInvalidSignalNumberText,
	/* 48 - invalid */		kInvalidSignalNumberText,
	/* 49 - invalid */		kInvalidSignalNumberText,
	/* 50 - invalid */		kInvalidSignalNumberText,
	/* 51 - invalid */		kInvalidSignalNumberText,
	/* 52 - invalid */		kInvalidSignalNumberText,
	/* 53 - invalid */		kInvalidSignalNumberText,
	/* 54 - invalid */		kInvalidSignalNumberText,
	/* 55 - invalid */		kInvalidSignalNumberText,
	/* 56 - invalid */		kInvalidSignalNumberText,
	/* 57 - invalid */		kInvalidSignalNumberText,
	/* 58 - invalid */		kInvalidSignalNumberText,
	/* 59 - invalid */		kInvalidSignalNumberText,
	/* 60 - invalid */		kInvalidSignalNumberText,
	/* 61 - invalid */		kInvalidSignalNumberText,
	/* 62 - invalid */		kInvalidSignalNumberText,
	/* 63 - invalid */		kInvalidSignalNumberText,
	/* 64 - invalid */		kInvalidSignalNumberText
};


const char*
strsignal(int sig)
{
	if (sig < 0 || sig > __MAX_SIGNO)
		return kInvalidSignalNumberText;

	return sys_siglist[sig];
}

