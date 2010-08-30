/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <platform/openfirmware/openfirmware.h>

#include "real_time_clock.h"


bigtime_t
system_time(void)
{
	int milliseconds = of_milliseconds();
	if (milliseconds == OF_FAILED)
		milliseconds = 0;
	bigtime_t result = milliseconds;
	int second, minute, hour, day, month, year;
	if (gRTC != OF_FAILED) {
		if (of_call_method(gRTC, "get-time", 0, 6, &year, &month, &day,
 				&hour, &minute, &second) != OF_FAILED) {
 			result %= 1000;
 			int days = day;
				// TODO: Apply algorithm from kernel
				// to assure monotonically increasing date.
 			result += (((days * 24 + hour) * 60ULL + minute) * 60ULL + second)
				* 1000ULL;
		}
	}
	return result * 1000;
}
