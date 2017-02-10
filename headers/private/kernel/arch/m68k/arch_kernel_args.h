/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
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
	int			cpu_type; // decimal: 68030, ...
	int			fpu_type; // decimal: 68030, ...
	int			mmu_type; // decimal: 68030, ...
	int			platform; // M68K_PLATFORM_*
	int			machine;  // platform specific machine type
	bool			has_lpstop; //XXX: use bit flags
	// architecture specific
	uint64		cpu_frequency;
	uint64		bus_frequency;
	uint64		time_base_frequency;

	// page root table
	uint32	phys_pgroot;
	uint32	vir_pgroot;
	// interrupt vector table (VBR)
	uint32	phys_vbr;
	uint32	vir_vbr;
//XXX:
	addr_range	page_table;		// virtual address and size of the page table
	addr_range	exception_handlers;
	addr_range	framebuffer;		// maps where the framebuffer is located, in physical memory
	int 		screen_x, screen_y, screen_depth;

	// The virtual ranges we want to keep in the kernel. E.g. those belonging
	// to the Open Firmware.
//	uint32		num_virtual_ranges_to_keep;
//	addr_range	virtual_ranges_to_keep[MAX_VIRTUAL_RANGES_TO_KEEP];

	// platform specific
	union {
		struct {
			struct {
				//XXX: are those meant to be used by physical
				// or virtual address ?
				uint32 (*nf_get_id)(const char *);
				int32 (*nf_call)(uint32, ...);
				addr_t nf_page;
				uint32 nf_dprintf_id;
			} nat_feat;
		} atari;
	} plat_args;
} arch_kernel_args;

#endif	/* KERNEL_ARCH_M68K_KERNEL_ARGS_H */
