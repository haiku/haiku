/*
 * Copyright 2025, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "util.h"


string
format_timespec(Context &context, timespec time)
{
	if ((time.tv_sec == 0 && time.tv_nsec == 0) || time.tv_nsec > 999999999)
		return "timespec error";
	time_t t = (time_t)time.tv_sec;
	const struct tm *timep = localtime(&t);
	char buf[64];
	size_t bytes = strftime(buf, sizeof(buf), "%FT%T", timep);
	if (bytes == 0)
		return "strftime error";
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "{tv_sec=%" B_PRIdTIME " /* %s */, tv_nsec=%ld}",
		time.tv_sec, buf, time.tv_nsec);
	return tmp;
}


string
format_unsigned(uint32 value)
{
	char tmp[12];
	snprintf(tmp, sizeof(tmp), "%" B_PRIu32, value);
	return tmp;
}


string
format_unsigned64(uint64 value)
{
	char tmp[24];
	snprintf(tmp, sizeof(tmp), "%" B_PRIu64, value);
	return tmp;
}

