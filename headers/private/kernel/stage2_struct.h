/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_BOOT_STAGE2_STRUCT_H
#define _NEWOS_BOOT_STAGE2_STRUCT_H

// must match SMP_MAX_CPUS in arch_smp.h
#define MAX_BOOT_CPUS 4
#define MAX_PHYS_MEM_ADDR_RANGE 4
#define MAX_VIRT_ALLOC_ADDR_RANGE 4
#define MAX_PHYS_ALLOC_ADDR_RANGE 4

typedef struct ar {
	unsigned long start;
	unsigned long size;
} addr_range;

#endif

