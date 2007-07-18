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
vm_cache *vm_cache_create(vm_store *store);
void vm_cache_acquire_ref(vm_cache *cache);
void vm_cache_release_ref(vm_cache *cache);
vm_page *vm_cache_lookup_page(vm_cache *cache, off_t page);
void vm_cache_insert_page(vm_cache *cache, vm_page *page, off_t offset);
void vm_cache_remove_page(vm_cache *cache, vm_page *page);
void vm_cache_remove_consumer(vm_cache *cache, vm_cache *consumer);
void vm_cache_add_consumer_locked(vm_cache *cache, vm_cache *consumer);
status_t vm_cache_write_modified(vm_cache *cache, bool fsReenter);
status_t vm_cache_set_minimal_commitment_locked(vm_cache *cache, off_t commitment);
status_t vm_cache_resize(vm_cache *cache, off_t newSize);
status_t vm_cache_insert_area_locked(vm_cache *cache, vm_area *area);
status_t vm_cache_remove_area(vm_cache *cache, vm_area *area);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_CACHE_H */
