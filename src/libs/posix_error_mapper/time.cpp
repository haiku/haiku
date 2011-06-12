/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, clock_nanosleep,
		(clockid_t clockID, int flags, const struct timespec* time,
			struct timespec* remainingTime),
	return B_TO_POSITIVE_ERROR(sReal_clock_nanosleep(clockID, flags, time,
		remainingTime));
)


WRAPPER_FUNCTION(int, clock_getcpuclockid,
		(pid_t pid, clockid_t* _clockID),
	return B_TO_POSITIVE_ERROR(sReal_clock_getcpuclockid(pid, _clockID));
)
