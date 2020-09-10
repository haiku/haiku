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
#include <arch/system_info.h>
#include <boot/kernel_args.h>


static uint64 sCPUClockFrequency;
static uint64 sBusClockFrequency;
static uint16 sCPURevision;


void
arch_fill_topology_node(cpu_topology_node_info* node, int32 cpu)
{
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_get_frequency(uint64 *frequency, int32 cpu)
{
	*frequency = sCPUClockFrequency;
	return B_OK;
}
