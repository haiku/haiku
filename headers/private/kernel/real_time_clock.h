/* 
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_REAL_TIME_CLOCK_H
#define _KERNEL_REAL_TIME_CLOCK_H


#include <KernelExport.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t rtc_init(kernel_args *ka);
	// Initialize the real-time clock.

bigtime_t rtc_boot_time(void);
	// Returns the time at which the system was booted in microseconds since Jan 1, 1970.

void _user_set_real_time_clock(uint32 time);

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_REAL_TIME_CLOCK_H */
