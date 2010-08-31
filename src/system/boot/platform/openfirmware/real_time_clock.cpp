/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "real_time_clock.h"

#include <stdio.h>

#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>


static int sHandle = OF_FAILED;


status_t
init_real_time_clock(void)
{
	// find RTC
	int rtcCookie = 0;
	if (of_get_next_device(&rtcCookie, 0, "rtc",
			gKernelArgs.platform_args.rtc_path,
			sizeof(gKernelArgs.platform_args.rtc_path)) != B_OK) {
		printf("init_real_time_clock(): Found no RTC device!");
		return B_ERROR;
	}

	sHandle = of_open(gKernelArgs.platform_args.rtc_path);
	if (sHandle == OF_FAILED) {
		printf("%s(): Could not open RTC device!\n", __func__);
		return B_ERROR;
	}

	return B_OK;
}


bigtime_t
real_time_clock_usecs(void)
{
	if (sHandle == OF_FAILED)
		return OF_FAILED;
	int second, minute, hour, day, month, year;
	if (of_call_method(sHandle, "get-time", 0, 6, &year, &month, &day,
			&hour, &minute, &second) == OF_FAILED)
		return OF_FAILED;
	int days = day;
		// TODO: Apply algorithm from kernel
		// to assure monotonically increasing date.
	return (((days * 24 + hour) * 60ULL + minute) * 60ULL + second)
		* 1000000ULL;
}
