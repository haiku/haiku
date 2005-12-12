/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "via.h"


static uint32
via_count_mtrrs(void)
{
	return 0;
}


static status_t
via_set_mtrr(uint32 index, addr_t base, addr_t length, uint32 type)
{
	return B_OK;
}


static status_t
via_unset_mtrr(uint32 index)
{
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
	via_set_mtrr,
	via_unset_mtrr,
};
