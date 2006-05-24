/*
 * Copyright 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "generic_x86.h"
#include "intel.h"
#include "amd.h"
#include "via.h"

#include <KernelExport.h>
#include <arch_system_info.h>


//#define TRACE_MTRR
#ifdef TRACE_MTRR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define IA32_MTRR_ENABLE				(1UL << 11)
#define IA32_MTRR_ENABLE_FIXED			(1UL << 10)
#define IA32_MTRR_VALID_RANGE			(1UL << 11)


struct mtrr_capabilities {
	mtrr_capabilities(uint64 value) { *(uint64 *)this = value; }

	uint64	variable_ranges : 8;
	uint64	supports_fixed : 1;
	uint64	_reserved0 : 1;
	uint64	supports_write_combined : 1;
	uint64	_reserved1 : 53;
};


uint64 gPhysicalMask = 0;


uint32
generic_count_mtrrs(void)
{
	cpuid_info cpuInfo;
	if (get_current_cpuid(&cpuInfo, 1) != B_OK
		|| (cpuInfo.eax_1.features & IA32_FEATURE_MTRR) == 0)
		return 0;

	mtrr_capabilities capabilities(x86_read_msr(IA32_MSR_MTRR_CAPABILITIES));
	TRACE(("CPU has %u variable range MTRs.\n", (uint8)capabilities.variable_ranges));
	return capabilities.variable_ranges;
}


void
generic_init_mtrrs(uint32 count)
{
	if (count == 0)
		return;

	// disable and clear all MTRRs
	// (we leave the fixed MTRRs as is)
	// TODO: check if the fixed MTRRs are set on all CPUs identically?
	TRACE(("generic_init_mtrrs(count = %ld)\n", count));

	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & ~IA32_MTRR_ENABLE);

	for (uint32 i = count; i-- > 0;) {
		if (x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + i * 2) & IA32_MTRR_VALID_RANGE)
			x86_write_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + i * 2, 0);
	}

	// but turn on variable MTRR functionality

	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) | IA32_MTRR_ENABLE);
}


void
generic_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type)
{
	index *= 2;
		// there are two registers per slot

	uint64 mask = length - 1;
	mask = ~mask & gPhysicalMask;

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


status_t
generic_get_mtrr(uint32 index, uint64 *_base, uint64 *_length, uint8 *_type)
{
	uint64 mask = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index * 2);
	if ((mask & IA32_MTRR_VALID_RANGE) == 0)
		return B_ERROR;

	uint64 base = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index * 2);

	*_base = base & ~(B_PAGE_SIZE - 1);
	*_length = (~mask & gPhysicalMask) + 1;
	*_type = base & 0xff;

	return B_OK;
}


status_t
generic_mtrr_compute_physical_mask(void)
{
	uint32 bits = 36;

	cpuid_info cpuInfo;
	if (get_cpuid(&cpuInfo, 0x80000000, 0) == B_OK
		&& cpuInfo.eax_0.max_eax & 0xff >= 8) {
		get_cpuid(&cpuInfo, 0x80000008, 0);
		bits = cpuInfo.regs.eax & 0xff;

		// Obviously, the bits are not always reported correctly
		if (bits < 36)
			bits = 36;
	}

	gPhysicalMask = ((1ULL << bits) - 1) & ~(B_PAGE_SIZE - 1);

	TRACE(("CPU has %ld physical address bits, physical mask is %016Lx\n",
		bits, gPhysicalMask));

	return B_OK;
}


void
generic_dump_mtrrs(uint32 count)
{
	if (x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & IA32_MTRR_ENABLE) {
		TRACE(("MTRR enabled\n"));
	} else {
		TRACE(("MTRR disabled\n"));
	}

	for (uint32 i = 0; i < count; i++) {
		uint64 base;
		uint64 length;
		uint8 type;
		if (generic_get_mtrr(i, &base, &length, &type) == B_OK) {
			TRACE(("  %ld: 0x%Lx, 0x%Lx, %u\n", i, base, length, type));
		} else {
			TRACE(("  %ld: empty\n", i));
		}
	}
}


module_info *modules[] = {
	(module_info *)&gIntelModule,
	(module_info *)&gAMDModule,
	(module_info *)&gVIAModule,
	NULL
};
