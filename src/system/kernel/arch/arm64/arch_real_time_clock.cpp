/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <arch/real_time_clock.h>

#include <real_time_clock.h>
#include <real_time_data.h>
#include <smp.h>


status_t
arch_rtc_init(kernel_args *args, struct real_time_data *data)
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
