/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/real_time_clock.h>

#include <real_time_clock.h>
#include <real_time_data.h>


status_t
arch_rtc_init(struct kernel_args *args, struct real_time_data *data)
{
	return B_OK;
}


uint32
arch_rtc_get_hw_time(void)
{
	return 0;
}


void
arch_rtc_set_hw_time(uint32 seconds)
{

}


void
arch_rtc_set_system_time_offset(struct real_time_data *data, bigtime_t offset)
{

}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data *data)
{
	return 0;
}
