/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "apm.h"
#include "bios.h"

#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>


//#define TRACE_APM
#ifdef TRACE_APM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
apm_init(void)
{
	// check if APM is available

	struct bios_regs regs;
	regs.eax = BIOS_APM_CHECK;
	regs.ebx = 0;
	call_bios(0x15, &regs);

	if ((regs.flags & CARRY_FLAG) != 0
		|| (regs.ebx & 0xffff) != 'PM') {
		dprintf("No APM available.\n");
		return B_ERROR;
	}

	apm_info &info = gKernelArgs.platform_args.apm;
	info.version = regs.eax & 0xffff;
	info.flags = regs.ecx & 0xffff;

	dprintf("APM version %d.%d available, flags %x.\n",
		(info.version >> 8) & 0xf, info.version & 0xf, info.flags);

	if ((info.version & 0xf) < 2) {
		// 32-bit protected mode interface was not available before 1.2
		return B_ERROR;
	}

	// there can always be one connection, so make sure we're
	// the one - and disconnect

	regs.eax = BIOS_APM_DISCONNECT;
	regs.ebx = 0;
	call_bios(0x15, &regs);
		// We don't care if this fails - there might not have been
		// any connection before.

	// try to connect to the 32-bit interface

	regs.eax = BIOS_APM_CONNECT_32_BIT;
	regs.ebx = 0;
	call_bios(0x15, &regs);
	if ((regs.flags & CARRY_FLAG) != 0) {
		// reset the version, so that the kernel won't try to use APM
		info.version = 0;
		return B_ERROR;
	}

	info.code32_segment_base = regs.eax & 0xffff;
	info.code32_segment_offset = regs.ebx;
	info.code32_segment_length = regs.esi & 0xffff;

	info.code16_segment_base = regs.ecx & 0xffff;
	info.code16_segment_length = regs.esi >> 16;

	info.data_segment_base = regs.edx & 0xffff;
	info.data_segment_length = regs.edi & 0xffff;

	TRACE(("  code32: 0x%x, 0x%lx, length 0x%x\n",
		info.code32_segment_base, info.code32_segment_offset, info.code32_segment_length));
	TRACE(("  code16: 0x%x, length 0x%x\n",
		info.code16_segment_base, info.code16_segment_length));
	TRACE(("  data: 0x%x, length 0x%x\n",
		info.data_segment_base, info.data_segment_length));

	return B_OK;
}
