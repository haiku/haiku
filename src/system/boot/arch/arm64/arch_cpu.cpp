/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#include <OS.h>
#include <boot/platform.h>
/*
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>
#include <string.h>
*/

extern "C" status_t
boot_arch_cpu_init(void)
{
	return B_OK;
}


extern "C" void
arch_ucode_load(BootVolume& volume)
{
	// Yes, we have no bananas!
}
