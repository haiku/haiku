/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_ARCH_M68K_KERNEL_ARGS_H
#define KERNEL_ARCH_M68K_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

#define _PACKED __attribute__((packed))

//#define MAX_VIRTUAL_RANGES_TO_KEEP	32

// kernel args
typedef struct {
	int			cpu_type; 
	int			fpu_type; 
	int			mmu_type; 
	int			platform; 
	int			machine;  // platform specific machine type
	bool			has_lpstop; //XXX: use bit flags
	// architecture specific
	uint64		cpu_frequency;
	uint64		bus_frequency;
	uint64		time_base_frequency;
} arch_kernel_args;

#endif	/* KERNEL_ARCH_M68K_KERNEL_ARGS_H */
