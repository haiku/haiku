/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_TIME_PRIVATE_H
#define _LIBROOT_TIME_PRIVATE_H


#include <errno.h>
#include <sys/cdefs.h>
#include <sys/time.h>
#include <time.h>

#include <SupportDefs.h>

#include <new>

#define CLOCKS_PER_SEC_BEOS					1000
#define CLK_TCK_BEOS						CLOCKS_PER_SEC_BEOS

#define MICROSECONDS_PER_CLOCK_TICK			(1000000 / CLOCKS_PER_SEC)
#define MICROSECONDS_PER_CLOCK_TICK_BEOS	(1000000 / CLOCKS_PER_SEC_BEOS)


struct __timer_t {
	int32		id;
	thread_id	thread;

	void SetTo(int32 id, thread_id thread)
	{
		this->id = id;
		this->thread = thread;
	}
};


static inline void
bigtime_to_timespec(bigtime_t time, timespec& spec)
{
	spec.tv_sec = time / 1000000;
	spec.tv_nsec = (time % 1000000) * 1000;
}


static inline bool
timespec_to_bigtime(const timespec& spec, bigtime_t& _time)
{
	if (spec.tv_sec < 0 || spec.tv_nsec < 0 || spec.tv_nsec >= 1000000000)
		return false;

	_time = (bigtime_t)spec.tv_sec * 1000000 + (spec.tv_nsec + 999) / 1000;

	return true;
}


static inline bool
timeval_to_timespec(const timeval& val, timespec& spec)
{
	if (val.tv_sec < 0 || val.tv_usec < 0 || val.tv_usec >= 1000000)
		return false;

	spec.tv_sec = val.tv_sec;
	spec.tv_nsec = val.tv_usec * 1000;

	return true;
}


static inline void
timespec_to_timeval(const timespec& spec, timeval& val)
{
	val.tv_sec = spec.tv_sec;
	val.tv_usec = spec.tv_nsec / 1000;
}


__BEGIN_DECLS


clock_t	__clock_beos(void);
clock_t	__clock(void);


__END_DECLS


#endif	// _LIBROOT_TIME_PRIVATE_H
