/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _STAGE2_H
#define _STAGE2_H

#include <boot.h>

// must match SMP_MAX_CPUS in arch_smp.h
#define MAX_BOOT_CPUS 1

#define MAX_PHYS_MEM_ADDR_RANGE 4
#define MAX_VIRT_ALLOC_ADDR_RANGE 4
#define MAX_PHYS_ALLOC_ADDR_RANGE 4

typedef struct {
	unsigned int start;
	unsigned int size;
} addr_range;

// kernel args
typedef struct {
	unsigned int cons_line;
	char *str;
	addr_range bootdir_addr;
	addr_range kernel_seg0_addr;	
	addr_range kernel_seg1_addr;	
	unsigned int num_phys_mem_ranges;
	addr_range phys_mem_range[MAX_PHYS_MEM_ADDR_RANGE];
	unsigned int num_phys_alloc_ranges;
	addr_range phys_alloc_range[MAX_PHYS_ALLOC_ADDR_RANGE];
	unsigned int num_virt_alloc_ranges;
	addr_range virt_alloc_range[MAX_VIRT_ALLOC_ADDR_RANGE];
	unsigned int num_cpus;
	addr_range cpu_kstack[MAX_BOOT_CPUS];
	// architecture specific
} kernel_args;

#endif

