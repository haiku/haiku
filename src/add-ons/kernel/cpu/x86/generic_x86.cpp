/*
 * Copyright 2005-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include "generic_x86.h"
#include "intel.h"
#include "amd.h"
#include "via.h"

#include <KernelExport.h>
#include <arch_system_info.h>
#include <arch/x86/arch_cpu.h>
#include <smp.h>


//#define TRACE_MTRR
#ifdef TRACE_MTRR
#	define TRACE(x...)	dprintf("mtrr: "x)
#else
#	define TRACE(x...)	/* nothing */
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


#ifdef TRACE_MTRR
static const char *
mtrr_type_to_string(uint8 type)
{
	switch (type) {
		case 0:
			return "uncacheable";
		case 1:
			return "write combining";
		case 4:
			return "write-through";
		case 5:
			return "write-protected";
		case 6:
			return "write-back";
		default:
			return "reserved";
	}
}
#endif // TRACE_MTRR


static void
set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type)
{
	uint64 mask = length - 1;
	mask = ~mask & gPhysicalMask;

	TRACE("MTRR %lu: new mask %Lx\n", index, mask);
	TRACE("  mask test base: %Lx\n", mask & base);
	TRACE("  mask test middle: %Lx\n", mask & (base + length / 2));
	TRACE("  mask test end: %Lx\n", mask & (base + length));

	index *= 2;
		// there are two registers per slot

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


// #pragma mark -


uint32
generic_count_mtrrs(void)
{
	if (!x86_check_feature(IA32_FEATURE_MTRR, FEATURE_COMMON)
		|| !x86_check_feature(IA32_FEATURE_MSR, FEATURE_COMMON))
		return 0;

	mtrr_capabilities capabilities(x86_read_msr(IA32_MSR_MTRR_CAPABILITIES));
	TRACE("CPU %ld has %u variable range MTRRs.\n", smp_get_current_cpu(),
		(uint8)capabilities.variable_ranges);
	return capabilities.variable_ranges;
}


void
generic_init_mtrrs(uint32 count)
{
	if (count == 0)
		return;

	// If MTRRs are enabled, we leave everything as is (save for, possibly, the
	// default, which we set below), so that we can benefit from the BIOS's
	// setup until we've installed our own. If MTRRs are disabled, we clear
	// all registers and enable MTRRs.
	// (we leave the fixed MTRRs as is)
	// TODO: check if the fixed MTRRs are set on all CPUs identically?
	TRACE("generic_init_mtrrs(count = %ld)\n", count);

	uint64 defaultType = x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE);
	if ((defaultType & IA32_MTRR_ENABLE) == 0) {
		for (uint32 i = 0; i < count; i++)
			set_mtrr(i, 0, 0, 0);
	}

	// Turn on variable MTRR functionality.
	// We need to ensure that the default type is uncacheable, otherwise
	// clearing the mtrrs could result in ranges that aren't supposed to be
	// cacheable to become cacheable due to the default type.
	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE,
		(defaultType & ~0xff) | IA32_MTRR_ENABLE);
}


void
generic_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type)
{
	set_mtrr(index, base, length, type);
	TRACE("[cpu %ld] mtrrs now:\n", smp_get_current_cpu());
	generic_dump_mtrrs(generic_count_mtrrs());
}


status_t
generic_get_mtrr(uint32 index, uint64 *_base, uint64 *_length, uint8 *_type)
{
	uint64 mask = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_MASK_0 + index * 2);
	if ((mask & IA32_MTRR_VALID_RANGE) == 0)
		return B_ERROR;

	uint64 base = x86_read_msr(IA32_MSR_MTRR_PHYSICAL_BASE_0 + index * 2);

	*_base = base & ~(B_PAGE_SIZE - 1);
	*_length = (~mask & gPhysicalMask) + B_PAGE_SIZE;
	*_type = base & 0xff;

	return B_OK;
}


void
generic_set_mtrrs(uint8 newDefaultType, const x86_mtrr_info* infos,
	uint32 count, uint32 maxCount)
{
	// check count
	if (maxCount == 0)
		return;

	if (count > maxCount)
		count = maxCount;

	// disable MTTRs
	uint64 defaultType = x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE)
		& ~IA32_MTRR_ENABLE;
	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE, defaultType);

	// set the given MTRRs
	for (uint32 i = 0; i < count; i++)
		set_mtrr(i, infos[i].base, infos[i].size, infos[i].type);

	// clear the other MTRRs
	for (uint32 i = count; i < maxCount; i++)
		set_mtrr(i, 0, 0, 0);

	// re-enable MTTRs and set the new default type
	defaultType = (defaultType & ~(uint64)0xff) | newDefaultType;
	x86_write_msr(IA32_MSR_MTRR_DEFAULT_TYPE, defaultType | IA32_MTRR_ENABLE);
}


status_t
generic_mtrr_compute_physical_mask(void)
{
	uint32 bits = 36;

	cpuid_info cpuInfo;
	if (get_current_cpuid(&cpuInfo, 0x80000000) == B_OK
		&& (cpuInfo.eax_0.max_eax & 0xff) >= 8) {
		get_current_cpuid(&cpuInfo, 0x80000008);
		bits = cpuInfo.regs.eax & 0xff;

		// Obviously, the bits are not always reported correctly
		if (bits < 36)
			bits = 36;
	}

	gPhysicalMask = ((1ULL << bits) - 1) & ~(B_PAGE_SIZE - 1);

	TRACE("CPU %ld has %ld physical address bits, physical mask is %016Lx\n",
		smp_get_current_cpu(), bits, gPhysicalMask);

	return B_OK;
}


void
generic_dump_mtrrs(uint32 count)
{
#ifdef TRACE_MTRR
	if (count == 0)
		return;

	int cpu = smp_get_current_cpu();
	uint64 defaultType = x86_read_msr(IA32_MSR_MTRR_DEFAULT_TYPE);
	TRACE("[cpu %d] MTRRs are %sabled\n", cpu,
		(defaultType & IA32_MTRR_ENABLE) != 0 ? "en" : "dis");
	TRACE("[cpu %d] default type is %u %s\n", cpu,
		(uint8)defaultType, mtrr_type_to_string(defaultType));
	TRACE("[cpu %d] fixed range MTRRs are %sabled\n", cpu,
		(defaultType & IA32_MTRR_ENABLE_FIXED) != 0 ? "en" : "dis");

	for (uint32 i = 0; i < count; i++) {
		uint64 base;
		uint64 length;
		uint8 type;
		if (generic_get_mtrr(i, &base, &length, &type) == B_OK) {
			TRACE("[cpu %d] %lu: base: 0x%Lx; length: 0x%Lx; type: %u %s\n",
				cpu, i, base, length, type, mtrr_type_to_string(type));
		} else
			TRACE("[cpu %d] %lu: empty\n", cpu, i);
	}
#endif // TRACE_MTRR
}


module_info *modules[] = {
	(module_info *)&gIntelModule,
	(module_info *)&gAMDModule,
	(module_info *)&gVIAModule,
	NULL
};
