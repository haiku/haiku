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


#define IA32_MTRR_FEATURE				(1UL << 12)
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


static uint32
intel_count_mtrrs(void)
{
	cpuid_info cpuInfo;
	if (get_cpuid(&cpuInfo, 1, 0) != B_OK
		|| (cpuInfo.eax_1.features & IA32_MTRR_FEATURE) == 0)
		return 0;

	mtrr_capabilities capabilities(x86_read_msr(IA32_MSR_MTRR_CAPABILITIES));
	return capabilities.variable_ranges;
}


static status_t
intel_enable_mtrrs(void)
{
	cpuid_info cpuInfo;
	if (get_cpuid(&cpuInfo, 1, 0) != B_OK
		|| (cpuInfo.eax_1.features & IA32_MTRR_FEATURE) == 0)
		return B_NOT_SUPPORTED;

	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) | IA32_MTRR_ENABLE);

	return B_OK;
}


static void
intel_disable_mtrrs(void)
{
	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & ~IA32_MTRR_ENABLE);
}


static void
intel_set_mtrr(uint32 index, addr_t base, addr_t length, uint32 type)
{
	if (base != 0 && length != 0) {
		// enable MTRR

		x86_write_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index,
			(base & ~(B_PAGE_SIZE - 1)) | type);
		x86_write_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index,
			(length & ~(B_PAGE_SIZE - 1)) | IA32_MTRR_VALID_RANGE);
	} else {
		// disable MTRR

		x86_write_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index, 0);
		x86_write_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index, 0);
	}
}


static status_t
intel_get_mtrr(uint32 index, addr_t *_base, addr_t *_length, uint32 *_type)
{
	uint64 base = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index);
	uint64 mask = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index);

	if ((mask & IA32_MTRR_VALID_RANGE) == 0)
		return B_ERROR;

	*_base = base & ~(B_PAGE_SIZE - 1);
	*_length = mask & ~(B_PAGE_SIZE - 1);
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
		|| (info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_AMD_x86)
		return B_ERROR;

	if (x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE) & IA32_MTRR_ENABLE)
		dprintf("MTRR enabled\n");
	else
		dprintf("MTRR disabled\n");

	for (uint32 i = 0; i < intel_count_mtrrs(); i++) {
		addr_t base;
		addr_t length;
		uint32 type;
		if (intel_get_mtrr(i, &base, &length, &type) == B_OK)
			dprintf("  %ld: %p, %p, %ld\n", i, (void *)base, (void *)length, type);
		else
			dprintf("  %ld: empty\n", i);
	}

// don't open it just now	
	return B_ERROR;
//	return B_OK;
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
	intel_enable_mtrrs,
	intel_disable_mtrrs,

	intel_set_mtrr,
	intel_get_mtrr,
};
