/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#define KERNEL_BOOT_KERNEL_ARGS_H


#include <SupportDefs.h>

#include <boot/elf.h>
#include <platform_kernel_args.h>
#include <arch_kernel_args.h>


#define CURRENT_KERNEL_ARGS_VERSION 1

typedef struct kernel_args {
	uint32		kernel_args_size;
	uint32		version;

	uint32		cons_line;
	char		*str;
	addr_range	bootdir_addr;

	struct preloaded_image kernel_image;
	struct preloaded_image *preloaded_images;

	uint32		num_physical_memory_ranges;
	addr_range	physical_memory_range[MAX_PHYSICAL_MEMORY_RANGE];
	uint32		num_physical_allocated_ranges;
	addr_range	physical_allocated_range[MAX_PHYSICAL_ALLOCATED_RANGE];
	uint32		num_virtual_allocated_ranges;
	addr_range	virtual_allocated_range[MAX_VIRTUAL_ALLOCATED_RANGE];

	uint32		num_cpus;
	addr_range	cpu_kstack[MAX_BOOT_CPUS];

	platform_kernel_args platform_args;
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

#endif	/* KERNEL_BOOT_KERNEL_ARGS_H */
