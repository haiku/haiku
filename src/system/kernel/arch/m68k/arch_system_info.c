/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>


static uint64 sCPUClockFrequency;
static uint64 sBusClockFrequency;
static enum cpu_types sCPUType;
static uint16 sCPURevision;

status_t
arch_get_system_info(system_info *info, size_t size)
{
	info->cpu_type = sCPUType;
	info->cpu_revision = sCPURevision;

	info->cpu_clock_speed = sCPUClockFrequency;
	info->bus_clock_speed = sBusClockFrequency;

	info->platform_type = B_MAC_PLATFORM;//XXX

	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	int i;

	sCPUClockFrequency = args->arch_args.cpu_frequency;
	sBusClockFrequency = args->arch_args.bus_frequency;

	sCPURevision = 0x68030; //XXX
	sCPUType = B_CPU_M68K;
	
	return B_OK;
}
