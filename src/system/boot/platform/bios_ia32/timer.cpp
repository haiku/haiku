/*
 * Copyright 2026, Haiku, Inc.. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>

#include "bios.h"


extern "C" bigtime_t
system_time()
{
	struct bios_regs regs;
	regs.eax = 0x00;
	call_bios(0x1A, &regs);
	if ((regs.flags & CARRY_FLAG) != 0)
		return 0;

	const uint64 kTicksPerDay = 0x1800b0;
	uint64 ticks = (((regs.ecx << 16) | regs.edx) + regs.eax * kTicksPerDay);

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
	if ((regs.flags & CARRY_FLAG) != 0)
		panic("BIOS doesn't support INT 15, AH 86");
}
