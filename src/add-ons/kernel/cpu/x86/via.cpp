/*
 * Copyright 2005-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "via.h"
#include "generic_x86.h"

#include <OS.h>


static uint32
via_count_mtrrs(void)
{
	if (!x86_check_feature(IA32_FEATURE_MTRR, FEATURE_COMMON))
		return 0;

	// IA32_MSR_MTRR_CAPABILITIES doesn't exist on VIA CPUs
	return 8;
}


static void
via_init_mtrrs(void)
{
	generic_init_mtrrs(via_count_mtrrs());
}


static void
via_set_mtrrs(uint8 defaultType, const x86_mtrr_info* infos, uint32 count)
{
	generic_set_mtrrs(defaultType, infos, count, via_count_mtrrs());
}


static status_t
via_init(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	if ((info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_VIA_IDT_x86)
		return B_ERROR;

	// current VIA CPUs have always 36 bit (or less?)
	gPhysicalMask = ((1ULL << 36) - 1) & ~(B_PAGE_SIZE - 1);

	generic_dump_mtrrs(generic_count_mtrrs());
	return B_OK;
}


static status_t
via_stdops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return via_init();
		case B_MODULE_UNINIT:
			return B_OK;
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

	generic_set_mtrr,
	generic_get_mtrr,
	via_set_mtrrs
};
