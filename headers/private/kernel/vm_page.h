/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_PAGE_H
#define _KERNEL_VM_PAGE_H


#include <vm.h>


struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

void vm_page_init_num_pages(kernel_args *args);
status_t vm_page_init(struct kernel_args *args);
status_t vm_page_init_post_area(struct kernel_args *args);
status_t vm_page_init_post_thread(struct kernel_args *args);

status_t vm_mark_page_inuse(addr_t page);
status_t vm_mark_page_range_inuse(addr_t startPage, addr_t length);
status_t vm_page_set_state(vm_page *page, int state);

// get some data about the number of pages in the system
size_t vm_page_num_pages(void);
size_t vm_page_num_free_pages(void);

status_t vm_page_write_modified_page(vm_cache *cache, struct vm_page *page,
	bool fsReenter);
status_t vm_page_write_modified_pages(vm_cache *cache, bool fsReenter);

void vm_page_unreserve_pages(uint32 count);
void vm_page_reserve_pages(uint32 count);

vm_page *vm_page_allocate_page(int pageState, bool reserved);
status_t vm_page_allocate_pages(int pageState, vm_page **pages, uint32 numPages);
vm_page *vm_page_allocate_page_run(int state, addr_t length);
vm_page *vm_page_at_index(int32 index);
vm_page *vm_lookup_page(addr_t pageNumber);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_PAGE_H */
