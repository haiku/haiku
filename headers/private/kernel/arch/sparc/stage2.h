/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _STAGE2_H
#define _STAGE2_H

#include <boot.h>

// kernel args
struct kernel_args {
	unsigned int cons_line;
	unsigned int mem_size;
	char *str;
	const boot_entry *bootdir;
	unsigned int bootdir_size;
	unsigned int kernel_seg0_base;
	unsigned int kernel_seg0_size;
	unsigned int kernel_seg1_base;
	unsigned int kernel_seg1_size;
	unsigned int phys_alloc_range_low;
	unsigned int phys_alloc_range_high;
	unsigned int virt_alloc_range_low;
	unsigned int virt_alloc_range_high;
	unsigned int stack_start;
	unsigned int stack_end;
	// architecture specific
};

#endif

