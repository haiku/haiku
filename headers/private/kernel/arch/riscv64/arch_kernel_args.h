/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_ARCH_RISCV64_KERNEL_ARGS_H
#define KERNEL_ARCH_RISCV64_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

// kernel args
typedef struct {
	// architecture specific
	uint64		phys_pgdir;
	uint64  	vir_pgdir;
	uint64		next_pagetable;
} arch_kernel_args;

#endif	/* KERNEL_ARCH_RISCV64_KERNEL_ARGS_H */
