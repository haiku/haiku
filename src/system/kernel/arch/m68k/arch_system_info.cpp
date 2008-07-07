/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <arch_platform.h>
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

	info->platform_type = M68KPlatform::Default()->PlatformType();

	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	int i;

	sCPUClockFrequency = args->arch_args.cpu_frequency;
	sBusClockFrequency = args->arch_args.bus_frequency;

	sCPURevision = args->arch_args.cpu_type; //XXX
#warning M68K: use 060 PCR[15:8]
	sCPUType = B_CPU_M68K;
	
	return B_OK;
}
