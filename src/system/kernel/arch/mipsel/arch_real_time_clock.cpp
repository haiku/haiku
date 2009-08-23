/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr.
 * Copyright 2006 Ingo Weinhold bonefish@cs.tu-berlin.de. All rights reserved.
 * Copyright 2003 Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <arch/real_time_clock.h>


status_t
arch_rtc_init(kernel_args* args, struct real_time_data* data)
{
#warning IMPLEMENT arch_rtc_init
	return B_ERROR;
}


uint32
arch_rtc_get_hw_time(void)
{
#warning IMPLEMENT arch_rtc_get_hw_time
	return 0;
}


void
arch_rtc_set_hw_time(uint32 seconds)
{
#warning IMPLEMENT arch_rtc_set_hw_time
}


void
arch_rtc_set_system_time_offset(struct real_time_data* data, bigtime_t offset)
{
#warning IMPLEMENT arch_rtc_set_system_time_offset
}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data* data)
{
#warning IMPLEMENT arch_rtc_get_system_time_offset
	return 0;
}

