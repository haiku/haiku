/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef KERNEL_BOOT_STAGE2_H
#define KERNEL_BOOT_STAGE2_H

// this file declares stuff like addr_range, MAX_*, etc.
#include <boot/stage2_struct.h>
#include <arch_stage2.h>


// kernel args
typedef struct kernel_args {
	uint32		cons_line;
	char		*str;
	addr_range	bootdir_addr;
	addr_range	kernel_seg0_addr;
	addr_range	kernel_seg1_addr;
	addr_range	kernel_dynamic_section_addr;
	uint32		num_phys_mem_ranges;

	addr_range	phys_mem_range[MAX_PHYS_MEM_ADDR_RANGE];
	uint32		num_phys_alloc_ranges;
	addr_range	phys_alloc_range[MAX_PHYS_ALLOC_ADDR_RANGE];
	uint32		num_virt_alloc_ranges;
	addr_range	virt_alloc_range[MAX_VIRT_ALLOC_ADDR_RANGE];
	uint32		num_cpus;
	addr_range	cpu_kstack[MAX_BOOT_CPUS];

	arch_kernel_args arch_args;

	struct framebuffer {
		int		enabled;
		int		x_size;
		int		y_size;
		int		bit_depth;
		int		already_mapped;
		addr_range mapping;
	} fb;
} kernel_args;

#endif	/* KERNEL_BOOT_STAGE2_H */
