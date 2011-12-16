/*
** Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
** Distributed under the terms of the MIT License.
*/

#include <OS.h>

#include <errno.h>
#include <sys/resource.h>
#include <sys/times.h>

#include <symbol_versioning.h>

#include <errno_private.h>
#include <time_private.h>
#include <times_private.h>


static inline clock_t
times_common(struct tms* buffer, bigtime_t microSecondsPerClock)
{
	team_usage_info info;
	status_t err;

	if ((err = get_team_usage_info(B_CURRENT_TEAM, RUSAGE_SELF, &info))
			!= B_OK) {
		__set_errno(err);
		return -1;
	}

	buffer->tms_utime = info.user_time / microSecondsPerClock;
	buffer->tms_stime = info.kernel_time / microSecondsPerClock;

	if ((err = get_team_usage_info(B_CURRENT_TEAM, RUSAGE_CHILDREN, &info))
			!= B_OK) {
		__set_errno(err);
		return -1;
	}

	buffer->tms_cutime = info.user_time / microSecondsPerClock;
	buffer->tms_cstime = info.kernel_time / microSecondsPerClock;

	return system_time() / microSecondsPerClock;
}


clock_t
__times_beos(struct tms* buffer)
{
	return times_common(buffer, MICROSECONDS_PER_CLOCK_TICK_BEOS);
}


clock_t
__times(struct tms* buffer)
{
	return times_common(buffer, MICROSECONDS_PER_CLOCK_TICK);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__times_beos", "times@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__times", "times@@", "1_ALPHA4");
