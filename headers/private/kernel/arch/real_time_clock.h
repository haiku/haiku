/* 
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_ARCH_REAL_TIME_CLOCK_H
#define KERNEL_ARCH_REAL_TIME_CLOCK_H


#include <kernel.h>


#ifdef __cplusplus
extern "C" {
#endif

void arch_rtc_set_hw_time(uint32 seconds);
	// Set HW clock to 'seconds' since 1/1/1970
uint32 arch_rtc_get_hw_time(void);
	// Returns number of seconds since 1/1/1970 as stored in HW

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_REAL_TIME_CLOCK_H */
