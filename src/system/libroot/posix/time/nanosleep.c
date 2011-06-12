/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>

#include <errno.h>

#include <syscall_utils.h>


int
nanosleep(const struct timespec* time, struct timespec* remainingTime)
{
	RETURN_AND_SET_ERRNO(
		clock_nanosleep(CLOCK_MONOTONIC, 0, time, remainingTime));
}

