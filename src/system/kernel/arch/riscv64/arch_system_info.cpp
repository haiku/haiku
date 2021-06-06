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
	switch (node->type) {
		case B_TOPOLOGY_ROOT:
			node->data.root.platform = B_CPU_RISC_V;
			break;

		case B_TOPOLOGY_PACKAGE:
			node->data.package.vendor = B_CPU_VENDOR_UNKNOWN;
			node->data.package.cache_line_size = CACHE_LINE_SIZE;
			break;

		case B_TOPOLOGY_CORE:
			node->data.core.model = 0;
			node->data.core.default_frequency = sCPUClockFrequency;
			break;

		default:
			break;
	}
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	#warning IMPLEMENT arch_system_info_init clock frequency
	sCPUClockFrequency = 1000000000;
	return B_OK;
}


status_t
arch_get_frequency(uint64 *frequency, int32 cpu)
{
	*frequency = sCPUClockFrequency;
	return B_OK;
}
