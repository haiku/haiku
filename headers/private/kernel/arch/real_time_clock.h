/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_REAL_TIME_CLOCK_H
#define KERNEL_ARCH_REAL_TIME_CLOCK_H


#include <kernel.h>

struct kernel_args;
struct real_time_data;


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_rtc_init(struct kernel_args *args, struct real_time_data *data);

void arch_rtc_set_hw_time(uint32 seconds);
	// Set HW clock to 'seconds' since 1/1/1970
uint32 arch_rtc_get_hw_time(void);
	// Returns number of seconds since 1/1/1970 as stored in HW

void arch_rtc_set_system_time_offset(struct real_time_data *data,
	bigtime_t offset);
	// Set the system time offset in data.
bigtime_t arch_rtc_get_system_time_offset(struct real_time_data *data);
	// Return the system time offset as stored in data.

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_REAL_TIME_CLOCK_H */
