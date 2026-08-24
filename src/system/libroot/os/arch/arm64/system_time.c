/*
 * Copyright 2019-2026 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>


bigtime_t
system_time(void)
{
	uint64 ticks;
	uint64 freq;
	// Prevent reordering which could cause time to jump back.
	arm64_isb();
	asm volatile("mrs %0, CNTVCT_EL0" : "=r"(ticks));
	asm volatile("mrs %0, CNTFRQ_EL0": "=r" (freq));

	return (ticks / freq) * 1000000 + (ticks % freq) * 1000000 / freq;
}
