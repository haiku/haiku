/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>


nanotime_t
system_time_nsecs()
{
	return system_time() * 1000;
}
