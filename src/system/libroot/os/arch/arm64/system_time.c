/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>
#include <real_time_data.h>


bigtime_t
system_time(void)
{
	uint64 ticks;
	uint64 freq;
	asm volatile("mrs %0, CNTPCT_EL0": "=r" (ticks));
	asm volatile("mrs %0, CNTFRQ_EL0": "=r" (freq));
	return (ticks * 1000000) / freq;
}
