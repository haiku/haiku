/* 
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_REAL_TIME_CLOCK_H
#define _KERNEL_REAL_TIME_CLOCK_H


#include <KernelExport.h>


int rtc_init(kernel_args *ka);
// Initialize the real-time clock.

void rtc_set_system_time(uint32 current_time);
// Set the system time.  'current_time' is the number of seconds elapsed since
// Jan. 1, 1970.

bigtime_t rtc_boot_time(void);
// Returns the time at which the system was booted in microseconds since Jan 1, 1970.

#endif /* _KERNEL_REAL_TIME_CLOCK_H */
