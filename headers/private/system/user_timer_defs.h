/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_USER_TIMER_DEFS_H
#define _SYSTEM_USER_TIMER_DEFS_H


#include <limits.h>
#include <time.h>

#include <SupportDefs.h>


#define CLOCK_PROCESS_USER_CPUTIME_ID	((clockid_t)-4)
	/* clock measuring the used user CPU time of the current process */

// limits
#define MAX_USER_TIMERS_PER_TEAM		_POSIX_TIMER_MAX
	// maximum numbers of user-defined user timers (timer_create())
#define MAX_USER_TIMER_OVERRUN_COUNT	INT_MAX
	// cap value of a timer's overrun counter

#if MAX_USER_TIMER_OVERRUN_COUNT < _POSIX_DELAYTIMER_MAX
#	error "MAX_USER_TIMER_OVERRUN_COUNT < _POSIX_DELAYTIMER_MAX"
#endif

#define USER_TIMER_REAL_TIME_ID				0
	// predefined ID for the real time timer
#define USER_TIMER_TEAM_TOTAL_TIME_ID		1
	// predefined ID for the team's total (kernel + user) time timer
#define USER_TIMER_TEAM_USER_TIME_ID		2
	// predefined ID for the team's user time timer
#define USER_TIMER_FIRST_USER_DEFINED_ID	3
	// first ID assigned to a user-defined timer (timer_create())

// _kern_create_user_timer() flag:
#define USER_TIMER_SIGNAL_THREAD			0x01
	// send the signal to the thread instead of the team (valid only for thread
	// timers)


struct user_timer_info {
	bigtime_t	remaining_time;
	bigtime_t	interval;
	uint32		overrun_count;
};


#endif	/* _SYSTEM_USER_TIMER_DEFS_H */
