/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_ARCH_SPARC_KERNEL_ARGS_H
#define KERNEL_ARCH_SPARC_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

#define _PACKED __attribute__((packed))

#define MAX_VIRTUAL_RANGES_TO_KEEP	32

// kernel args
typedef struct {
	// architecture specific
	uint64		cpu_frequency;
	uint64		bus_frequency;
	uint64		time_base_frequency;

	addr_range	framebuffer;		// maps where the framebuffer is located, in physical memory
	int 		screen_x, screen_y, screen_depth;

	// The virtual ranges we want to keep in the kernel. E.g. those belonging
	// to the Open Firmware.
	uint32		num_virtual_ranges_to_keep;
	addr_range	virtual_ranges_to_keep[MAX_VIRTUAL_RANGES_TO_KEEP];

	// platform type we booted from
	int			platform;
} arch_kernel_args;

#endif	/* KERNEL_ARCH_SPARC_KERNEL_ARGS_H */

