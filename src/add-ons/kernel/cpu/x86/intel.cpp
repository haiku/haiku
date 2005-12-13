/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "intel.h"

#include <KernelExport.h>
#include <OS.h>
#include <arch_cpu.h>


//#define TRACE_MTRR
#ifdef TRACE_MTRR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define IA32_MTRR_FEATURE				(1UL << 12)
#define IA32_MTRR_ENABLE				(1UL << 11)
#define IA32_MTRR_ENABLE_FIXED			(1UL << 10)
#define IA32_MTRR_VALID_RANGE			(1UL << 11)

#define	MTRR_MASK	(0xffffffff & (B_PAGE_SIZE - 1))

struct mtrr_capabilities {
	mtrr_capabilities(uint64 value) { *(uint64 *)this = value; }

	uint64	variable_ranges : 8;
	uint64	supports_fixed : 1;
	uint64	_reserved0 : 1;
	uint64	supports_write_combined : 1;
	uint64	_reserved1 : 53;
};


static uint64 sPhysicalMask = 0;


static uint32
intel_count_mtrrs(void)
{
	cpuid_info cpuInfo;
	if (get_cpuid(&cpuInfo, 1, 0) != B_OK
		|| (cpuInfo.eax_1.features & IA32_MTRR_FEATURE) == 0)
		return 0;

	mtrr_capabilities capabilities(x86_read_msr(IA32_MSR_MTRR_CAPABILITIES));
	TRACE(("cpu has %ld variable range MTRs.\n", capabilities.variable_ranges));
	return capabilities.variable_ranges;
}


static void
intel_init_mtrrs(void)
{
	// disable and clear all MTRRs
	// (we leave the fixed MTRRs as is)
	// TODO: check if the fixed MTRRs are set on all CPUs identically?

	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & ~IA32_MTRR_ENABLE);

	for (uint32 i = intel_count_mtrrs(); i-- > 0;) {
		if (x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + i * 2) & IA32_MTRR_VALID_RANGE)
			x86_write_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + i * 2, 0);
	}

	// but turn on variable MTRR functionality

	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) | IA32_MTRR_ENABLE);
}


static void
intel_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type)
{
	index *= 2;
		// there are two registers per slot

	uint64 mask = length - 1;
	mask = ~mask & sPhysicalMask;

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
intel_get_mtrr(uint32 index, uint64 *_base, uint64 *_length, uint8 *_type)
{
	uint64 mask = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index * 2);
	if ((mask & IA32_MTRR_VALID_RANGE) == 0)
		return B_ERROR;

	uint64 base = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index * 2);

	*_base = base & ~(B_PAGE_SIZE - 1);
	*_length = (~mask & sPhysicalMask) + 1;
	*_type = base & 0xff;

	return B_OK;
}


static status_t
intel_init(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	if ((info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_INTEL_x86
		&& (info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_AMD_x86)
		return B_ERROR;

	if (x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & IA32_MTRR_ENABLE) {
		TRACE(("MTRR enabled\n"));
	} else {
		TRACE(("MTRR disabled\n"));
	}

	for (uint32 i = 0; i < intel_count_mtrrs(); i++) {
		uint64 base;
		uint64 length;
		uint8 type;
		if (intel_get_mtrr(i, &base, &length, &type) == B_OK) {
			TRACE(("  %ld: 0x%Lx, 0x%Lx, %u\n", i, base, length, type));
		} else {
			TRACE(("  %ld: empty\n", i));
		}
	}

	// TODO: dump fixed ranges as well

	// get number of physical address bits

	uint32 bits = 36;

	cpuid_info cpuInfo;
	if (get_cpuid(&cpuInfo, 0x80000000, 0) == B_OK
		&& cpuInfo.eax_0.max_eax & 0xff >= 8) {
		get_cpuid(&cpuInfo, 0x80000008, 0);
		bits = cpuInfo.regs.eax & 0xff;
	}

	sPhysicalMask = ((1ULL << bits) - 1) & ~(B_PAGE_SIZE - 1);

	TRACE(("CPU has %ld physical address bits, physical mask is %016Lx\n",
		bits, sPhysicalMask));

	return B_OK;
}


static status_t
intel_uninit(void)
{
	return B_OK;
}


static status_t
intel_stdops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return intel_init();
		case B_MODULE_UNINIT:
			return intel_uninit();
	}

	return B_ERROR;
}


x86_cpu_module_info gIntelModule = {
	{
		"cpu/generic_x86/intel/v1",
		0,
		intel_stdops,
	},

	intel_count_mtrrs,
	intel_init_mtrrs,

	intel_set_mtrr,
	intel_get_mtrr,
};
