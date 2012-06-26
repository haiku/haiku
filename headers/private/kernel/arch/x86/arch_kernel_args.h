/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_x86_KERNEL_ARGS_H
#define KERNEL_ARCH_x86_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif


#include <util/FixedWidthPointer.h>


#define MAX_BOOT_PTABLES 4

#define _PACKED __attribute__((packed))

#define IDT_LIMIT 0x800
#define GDT_LIMIT 0x800

// kernel args
typedef struct {
	// architecture specific
	uint32	system_time_cv_factor;
	uint64	cpu_clock_speed;
	uint32	phys_pgdir;
	uint64	vir_pgdir;
	uint32	num_pgtables;
	uint32	pgtables[MAX_BOOT_PTABLES];
	uint64	virtual_end;
	uint32	phys_idt;
	uint64	vir_idt;
	uint32	phys_gdt;
	uint64	vir_gdt;
	uint64	page_hole;
	// smp stuff
	uint32	apic_time_cv_factor; // apic ticks per second
	uint32	apic_phys;
	FixedWidthPointer<void> apic;
	uint32	ioapic_phys;
	uint32	cpu_apic_id[MAX_BOOT_CPUS];
	uint32	cpu_apic_version[MAX_BOOT_CPUS];
	// hpet stuff
	uint32	hpet_phys;
	FixedWidthPointer<void> hpet;
} _PACKED arch_kernel_args;

#endif	/* KERNEL_ARCH_x86_KERNEL_ARGS_H */
