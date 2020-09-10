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


status_t
arch_get_system_info(system_info *info, size_t size)
{
	return B_OK;
}


void
arch_fill_topology_node(cpu_topology_node_info* node, int32 cpu)
{
	switch (node->type) {
		case B_TOPOLOGY_ROOT:
			// TODO: ARM_64?
			node->data.root.platform = B_CPU_ARM;
			break;

		case B_TOPOLOGY_PACKAGE:
			//TODO node->data.package.vendor = sCPUVendor;
			node->data.package.cache_line_size = CACHE_LINE_SIZE;
			break;

		case B_TOPOLOGY_CORE:
			//TODO node->data.core.model = sPVR;
			//TODO node->data.core.default_frequency = sCPUClockFrequency;
			break;

		default:
			break;
	}
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_get_frequency(uint64 *frequency, int32 cpu)
{
	*frequency = 0;
	return B_OK;
}

