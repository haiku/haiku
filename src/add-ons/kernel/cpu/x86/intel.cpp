/*
 * Copyright 2005-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "intel.h"
#include "generic_x86.h"

#include <OS.h>


static void
intel_init_mtrrs(void)
{
	generic_init_mtrrs(generic_count_mtrrs());
}


static status_t
intel_init(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	if ((info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_INTEL_x86)
		return B_ERROR;

	generic_mtrr_compute_physical_mask();
	generic_dump_mtrrs(generic_count_mtrrs());

	return B_OK;
}


static void
intel_set_mtrrs(uint8 defaultType, const x86_mtrr_info* infos, uint32 count)
{
	generic_set_mtrrs(defaultType, infos, count, generic_count_mtrrs());
}


static status_t
intel_stdops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return intel_init();
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


x86_cpu_module_info gIntelModule = {
	{
		"cpu/generic_x86/intel/v1",
		0,
		intel_stdops,
	},

	generic_count_mtrrs,
	intel_init_mtrrs,

	generic_set_mtrr,
	generic_get_mtrr,
	intel_set_mtrrs
};
