/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <OS.h>

#include <kernel.h>
#include <smp.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>

#include <string.h>


status_t
arch_get_system_info(system_info *info, size_t size)
{
	//info->cpu_type = sCpuType;
	//info->cpu_revision = sCpuRevision;

	// - various cpu_info
	//info->cpu_clock_speed = sCpuClockSpeed;
	// - bus_clock_speed
	info->platform_type = B_MAC_PLATFORM;

	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	//sCpuClockSpeed = args->arch_args.cpu_clock_speed;
	return B_OK;
}

