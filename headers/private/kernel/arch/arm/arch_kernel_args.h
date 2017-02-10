/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_ARCH_ARM_KERNEL_ARGS_H
#define KERNEL_ARCH_ARM_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

// kernel args
typedef struct {
	int		cpu_type; 
	int		fpu_type; 
	int		mmu_type; 
	int		platform; 
	int		machine;  // platform specific machine type

	// architecture specific
        uint32  	phys_pgdir;
        uint32  	vir_pgdir;
	uint32		next_pagetable;
} arch_kernel_args;

#endif	/* KERNEL_ARCH_ARM_KERNEL_ARGS_H */
