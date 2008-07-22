/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_CACHE_H
#define _KERNEL_VM_CACHE_H


#include <kernel.h>
#include <vm.h>


struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

status_t vm_cache_init(struct kernel_args *args);
struct VMCache *vm_cache_acquire_locked_page_cache(struct vm_page *page,
	bool dontWait);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_CACHE_H */
