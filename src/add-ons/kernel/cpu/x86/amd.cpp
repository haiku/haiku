/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "amd.h"


static uint32
amd_count_mtrrs(void)
{
	return 0;
}


static status_t
amd_set_mtrr(uint32 index, addr_t base, addr_t length, uint32 type)
{
	return B_OK;
}


static status_t
amd_unset_mtrr(uint32 index)
{
	return B_OK;
}


static status_t
amd_init(void)
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return B_ERROR;

	if ((info.cpu_type & B_CPU_x86_VENDOR_MASK) != B_CPU_AMD_x86)
		return B_ERROR;

	return B_OK;
}


static status_t
amd_uninit(void)
{
	return B_OK;
}


static status_t
amd_stdops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return amd_init();
		case B_MODULE_UNINIT:
			return amd_uninit();
	}

	return B_ERROR;
}


x86_cpu_module_info gAMDModule = {
	{
		"cpu/generic_x86/amd/v1",
		0,
		amd_stdops,
	},

	amd_count_mtrrs,
	amd_set_mtrr,
	amd_unset_mtrr,
};
