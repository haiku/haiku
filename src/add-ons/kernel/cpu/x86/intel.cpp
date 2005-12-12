/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "intel.h"


static uint32
intel_count_mtrrs(void)
{
	return 0;
}


static status_t
intel_set_mtrr(uint32 index, addr_t base, addr_t length, uint32 type)
{
	return B_OK;
}


static status_t
intel_unset_mtrr(uint32 index)
{
	return B_OK;
}


static status_t
intel_init(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	if ((info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_INTEL_x86)
		return B_ERROR;

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
	intel_set_mtrr,
	intel_unset_mtrr,
};
