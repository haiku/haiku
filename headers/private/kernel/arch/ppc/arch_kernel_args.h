/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_ARCH_PPC_KERNEL_ARGS_H
#define KERNEL_ARCH_PPC_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

#define _PACKED __attribute__((packed))

// kernel args
typedef struct {
	// architecture specific
	addr_range page_table;	// maps where the page table is located, in physical memory
	addr_range framebuffer;	// maps where the framebuffer is located, in physical memory
	int screen_x, screen_y, screen_depth;
} arch_kernel_args;

#endif	/* KERNEL_ARCH_PPC_KERNEL_ARGS_H */
