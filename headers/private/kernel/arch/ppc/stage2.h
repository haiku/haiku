/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _PPC_STAGE2_H
#define _PPC_STAGE2_H

#include <boot/stage2_struct.h>

// kernel args
typedef struct {
	// architecture specific
	addr_range page_table; // maps where the page table is located, in physical memory
	addr_range framebuffer; // maps where the framebuffer is located, in physical memory
	int screen_x, screen_y, screen_depth;
} arch_kernel_args;

#endif

