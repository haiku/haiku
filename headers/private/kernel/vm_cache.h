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

//typedef struct vm_store vm_store;


#ifdef __cplusplus
extern "C" {
#endif

status_t vm_cache_init(struct kernel_args *args);
struct vm_cache *vm_cache_create(struct vm_store *store);
void vm_cache_acquire_ref(struct vm_cache *cache);
void vm_cache_release_ref(struct vm_cache *cache);
struct vm_cache *vm_cache_acquire_page_cache_ref(struct vm_page *page);
struct vm_page *vm_cache_lookup_page(struct vm_cache *cache, off_t page);
void vm_cache_insert_page(struct vm_cache *cache, struct vm_page *page,
	off_t offset);
void vm_cache_remove_page(struct vm_cache *cache, struct vm_page *page);
void vm_cache_remove_consumer(struct vm_cache *cache, struct vm_cache *consumer);
void vm_cache_add_consumer_locked(struct vm_cache *cache,
	struct vm_cache *consumer);
status_t vm_cache_write_modified(struct vm_cache *cache, bool fsReenter);
status_t vm_cache_set_minimal_commitment_locked(struct vm_cache *cache,
	off_t commitment);
status_t vm_cache_resize(struct vm_cache *cache, off_t newSize);
status_t vm_cache_insert_area_locked(struct vm_cache *cache, vm_area *area);
status_t vm_cache_remove_area(struct vm_cache *cache, struct vm_area *area);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_CACHE_H */
