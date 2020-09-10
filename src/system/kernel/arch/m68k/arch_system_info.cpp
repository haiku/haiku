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
static uint16 sCPURevision;


void
arch_fill_topology_node(cpu_topology_node_info* node, int32 cpu)
{
	switch (node->type) {
		case B_TOPOLOGY_ROOT:
			node->data.root.platform = B_CPU_M68K;
			break;

		case B_TOPOLOGY_PACKAGE:
			node->data.package.vendor = B_CPU_VENDOR_MOTOROLA;
			node->data.package.cache_line_size = CACHE_LINE_SIZE;
			break;

		case B_TOPOLOGY_CORE:
			node->data.core.model = sCPURevision;
			node->data.core.default_frequency = sCPUClockFrequency;
			break;

		default:
			break;
	}
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	sCPUClockFrequency = args->arch_args.cpu_frequency;
	sBusClockFrequency = args->arch_args.bus_frequency; // not reported anymore?

	sCPURevision = args->arch_args.cpu_type; //TODO:is it what we want?
#warning M68K: use 060 PCR[15:8]
	
	return B_OK;
}


status_t
arch_get_frequency(uint64 *frequency, int32 cpu)
{
	*frequency = sCPUClockFrequency;
	return B_OK;
}
