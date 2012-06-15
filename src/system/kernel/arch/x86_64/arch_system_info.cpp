/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/system_info.h>


status_t
arch_get_system_info(system_info *info, size_t size)
{
	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	return B_OK;
}
