/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
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
