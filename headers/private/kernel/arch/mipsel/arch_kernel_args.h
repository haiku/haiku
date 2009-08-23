/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_MIPSEL_KERNEL_ARGS_H
#define _KERNEL_ARCH_MIPSEL_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

#warning IMPLEMENT arch_kernel_args.h

#define _PACKED __attribute__((packed))

// kernel args
typedef struct {
	int			cpu_type;
	int			fpu_type;
	int			mmu_type;
	int			platform;
	int			machine; 

	// architecture specific
	uint64		cpu_frequency;
	uint64		bus_frequency;
	uint64		time_base_frequency;
} arch_kernel_args;

#endif /* _KERNEL_ARCH_MIPSEL_KERNEL_ARGS_H */

