/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_BOOT_ARCH_I386_STAGE2_H
#define _NEWOS_KERNEL_BOOT_ARCH_I386_STAGE2_H

#include <stage2_struct.h>


#define MAX_BOOT_PTABLES 4

// kernel args
typedef struct {
	// architecture specific
	unsigned int system_time_cv_factor;
	unsigned int phys_pgdir;
	unsigned int vir_pgdir;
	unsigned int num_pgtables;
	unsigned int pgtables[MAX_BOOT_PTABLES];
	unsigned int phys_idt;
	unsigned int vir_idt;
	unsigned int phys_gdt;
	unsigned int vir_gdt;
	unsigned int page_hole;
	// smp stuff
	unsigned int apic_time_cv_factor; // apic ticks per second
	unsigned int apic_phys;
	unsigned int *apic;
	unsigned int ioapic_phys;
	unsigned int *ioapic;
	unsigned int cpu_apic_id[MAX_BOOT_CPUS];
	unsigned int cpu_os_id[MAX_BOOT_CPUS];
	unsigned int cpu_apic_version[MAX_BOOT_CPUS];
} arch_kernel_args;

#endif

