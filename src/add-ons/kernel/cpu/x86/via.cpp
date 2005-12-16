/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "via.h"

#include <KernelExport.h>
#include <OS.h>
#include <arch_cpu.h>


#define TRACE_MTRR
#ifdef TRACE_MTRR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define IA32_MTRR_ENABLE				(1UL << 11)
#define IA32_MTRR_ENABLE_FIXED			(1UL << 10)
#define IA32_MTRR_VALID_RANGE			(1UL << 11)


static uint64 kPhysicalMask = ((1ULL << 36) - 1) & ~(B_PAGE_SIZE - 1);;


static uint32
via_count_mtrrs(void)
{
	cpuid_info cpuInfo;
	if (get_cpuid(&cpuInfo, 1, 0) != B_OK
		|| (cpuInfo.eax_1.features & IA32_FEATURE_MTRR) == 0)
		return 0;

	// IA32_MSR_MTRR_CAPABILITIES doesn't exist on VIA CPUs
	return 8;
}


static void
via_init_mtrrs(void)
{
	// disable and clear all MTRRs
	// (we leave the fixed MTRRs as is)
	// TODO: check if the fixed MTRRs are set on all CPUs identically?

	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & ~IA32_MTRR_ENABLE);

	for (uint32 i = via_count_mtrrs(); i-- > 0;) {
		if (x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + i * 2) & IA32_MTRR_VALID_RANGE)
			x86_write_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + i * 2, 0);
	}

	// but turn on variable MTRR functionality

	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) | IA32_MTRR_ENABLE);
}


static void
via_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type)
{
	index *= 2;
		// there are two registers per slot

	uint64 mask = length - 1;
	mask = ~mask & kPhysicalMask;

	TRACE(("MTRR %ld: new mask %Lx)\n", index, mask));
	TRACE(("  mask test base: %Lx)\n", mask & base));
	TRACE(("  mask test middle: %Lx)\n", mask & (base + length / 2)));
	TRACE(("  mask test end: %Lx)\n", mask & (base + length)));

	// First, disable MTRR

	x86_write_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index, 0);

	if (base != 0 || mask != 0 || type != 0) {
		// then fill in the new values, and enable it again

		x86_write_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index,
			(base & ~(B_PAGE_SIZE - 1)) | type);
		x86_write_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index,
			mask | IA32_MTRR_VALID_RANGE);
	} else {
		// reset base as well
		x86_write_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index, 0);
	}
}


static status_t
via_get_mtrr(uint32 index, uint64 *_base, uint64 *_length, uint8 *_type)
{
	uint64 mask = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index * 2);
	if ((mask & IA32_MTRR_VALID_RANGE) == 0)
		return B_ERROR;

	uint64 base = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index * 2);

	*_base = base & ~(B_PAGE_SIZE - 1);
	*_length = (~mask & kPhysicalMask) + 1;
	*_type = base & 0xff;

	return B_OK;
}


static status_t
via_init(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	if ((info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_VIA_IDT_x86)
		return B_ERROR;

	if (x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & IA32_MTRR_ENABLE) {
		TRACE(("MTRR enabled\n"));
	} else {
		TRACE(("MTRR disabled\n"));
	}

	for (uint32 i = 0; i < via_count_mtrrs(); i++) {
		uint64 base;
		uint64 length;
		uint8 type;
		if (via_get_mtrr(i, &base, &length, &type) == B_OK) {
			TRACE(("  %ld: 0x%Lx, 0x%Lx, %u\n", i, base, length, type));
		} else {
			TRACE(("  %ld: empty\n", i));
		}
	}

	// TODO: dump fixed ranges as well

	return B_OK;
}


static status_t
via_uninit(void)
{
	return B_OK;
}


static status_t
via_stdops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return via_init();
		case B_MODULE_UNINIT:
			return via_uninit();
	}

	return B_ERROR;
}


x86_cpu_module_info gVIAModule = {
	{
		"cpu/generic_x86/via/v1",
		0,
		via_stdops,
	},

	via_count_mtrrs,
	via_init_mtrrs,

	via_set_mtrr,
	via_get_mtrr,
};
