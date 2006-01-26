/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "real_time_clock.h"

#include <stdio.h>

#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>

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

	return B_OK;
}

