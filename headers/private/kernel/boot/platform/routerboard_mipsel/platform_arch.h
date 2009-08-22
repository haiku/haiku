/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_BOOT_PLATFORM_ROUTERBOARD_MIPSEL_ARCH_H
#define _KERNEL_BOOT_PLATFORM_ROUTERBOARD_MIPSEL_ARCH_H

#include <SupportDefs.h>

#warning IMPLEMENT platform_arch.h

struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

/* memory management */
	
extern status_t arch_set_callback(void);
extern void *arch_mmu_allocate(void *address, size_t size, uint8 protection,
	bool exactAddress);
extern status_t arch_mmu_free(void *address, size_t size);
extern status_t arch_mmu_init(void);

/* CPU */

extern status_t boot_arch_cpu_init(void);

/* kernel start */

status_t arch_start_kernel(struct kernel_args *kernelArgs, addr_t kernelEntry,
	addr_t kernelStackTop);

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_BOOT_PLATFORM_ROUTERBOARD_MIPSEL_ARCH_H */

