/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SIGNAL_DEFS_H
#define _SYSTEM_SIGNAL_DEFS_H


#include <signal.h>
#include <unistd.h>


// The total number of signals a process may have queued at receivers at any
// time.
#define MAX_QUEUED_SIGNALS			_POSIX_SIGQUEUE_MAX

// realtime signal number range
#define SIGNAL_REALTIME_MIN			33
#define SIGNAL_REALTIME_MAX			40

// greatest actually supported signal number
#define MAX_SIGNAL_NUMBER			SIGNAL_REALTIME_MAX

// additional send_signal_etc() flags
#define SIGNAL_FLAG_QUEUING_REQUIRED		(0x10000)
	// force signal queuing, i.e. fail instead of falling back to unqueued
	// signals, when queuing isn't possible
#define SIGNAL_FLAG_SEND_TO_THREAD			(0x20000)
	// interpret the the given ID as a thread_id rather than a team_id (syscall
	// use only)

// additional sigaction::sa_flags flag
#define SA_BEOS_COMPATIBLE_HANDLER		0x80000000
	// BeOS compatible signal handler, i.e. the vregs argument is passed
	// per-value, not per-pointer

#define SIGNAL_TO_MASK(signal)	((sigset_t)1 << ((signal) - 1))
#define SIGNAL_RANGE_TO_MASK(first, last)	\
	((((SIGNAL_TO_MASK(last) - 1) << 1) | 1) & ~(SIGNAL_TO_MASK(first) - 1))
	// Note: The last mask computation looks that way to avoid an overflow for
	// last == 64.

#endif	/* _SYSTEM_SIGNAL_DEFS_H */
