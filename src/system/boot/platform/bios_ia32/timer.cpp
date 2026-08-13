/*
 * Copyright 2026, Haiku, Inc.. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <kernel.h>

#include "bios.h"


static uint64
get_ticks()
{
	struct bios_regs regs;
	regs.eax = 0x00;
	call_bios(0x1A, &regs);
	if ((regs.flags & CARRY_FLAG) != 0)
		return 0;

	const uint64 kTicksPerDay = 0x1800b0;
	return (((regs.ecx << 16) | regs.edx) + regs.eax * kTicksPerDay);
}


extern "C" bigtime_t
system_time()
{
	uint64 ticks = get_ticks();

	// Convert to microseconds. Algebraically simplified version of:
	// ticks * (1000 * 1000) / (kTicksPerDay / (24 * 60 * 60))
	return (ticks * 1080000000) / 19663;
}


extern "C" void
spin(bigtime_t microseconds)
{
	struct bios_regs regs;
	regs.eax = 0x8600;
	regs.edx = microseconds & 0xFFFF;
	regs.ecx = microseconds << 16;
	call_bios(0x15, &regs);

	if ((regs.flags & CARRY_FLAG) != 0) {
		// Fall back to INT 1A. (Unfortunately this has a granularity of ~54.9ms.)
		uint64 target = get_ticks();
		if (target == 0)
			return;

		uint64 ticks = HOWMANY(microseconds, 55 * 1000);
		if (ticks < 2)
			ticks = 2;

		target += ticks;
		while (get_ticks() < target)
			asm volatile("pause");
	}
}
