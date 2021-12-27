/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>

#include "mmu.h"


void
arch_convert_kernel_args(void)
{
	fix_address(gKernelArgs.ucode_data);
	fix_address(gKernelArgs.arch_args.apic);
	fix_address(gKernelArgs.arch_args.hpet);
}


void
arch_start_kernel(addr_t kernelEntry)
{
}
