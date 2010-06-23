/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_PAGE_H
#define _KERNEL_VM_VM_PAGE_H


#include <vm/vm.h>
#include <vm/vm_types.h>


struct kernel_args;

extern int32 gMappedPagesCount;


struct vm_page_reservation {
	uint32	count;
};


#ifdef __cplusplus
extern "C" {
#endif

void vm_page_init_num_pages(struct kernel_args *args);
status_t vm_page_init(struct kernel_args *args);
status_t vm_page_init_post_area(struct kernel_args *args);
status_t vm_page_init_post_thread(struct kernel_args *args);

status_t vm_mark_page_inuse(page_num_t page);
status_t vm_mark_page_range_inuse(page_num_t startPage, page_num_t length);
void vm_page_free(struct VMCache *cache, struct vm_page *page);
void vm_page_set_state(struct vm_page *page, int state);
void vm_page_requeue(struct vm_page *page, bool tail);

// get some data about the number of pages in the system
page_num_t vm_page_num_pages(void);
page_num_t vm_page_num_free_pages(void);
page_num_t vm_page_num_available_pages(void);
page_num_t vm_page_num_unused_pages(void);
void vm_page_get_stats(system_info *info);
phys_addr_t vm_page_max_address();

status_t vm_page_write_modified_page_range(struct VMCache *cache,
	uint32 firstPage, uint32 endPage);
status_t vm_page_write_modified_pages(struct VMCache *cache);
void vm_page_schedule_write_page(struct vm_page *page);
void vm_page_schedule_write_page_range(struct VMCache *cache,
	uint32 firstPage, uint32 endPage);

void vm_page_unreserve_pages(vm_page_reservation* reservation);
void vm_page_reserve_pages(vm_page_reservation* reservation, uint32 count,
	int priority);
bool vm_page_try_reserve_pages(vm_page_reservation* reservation, uint32 count,
	int priority);

struct vm_page *vm_page_allocate_page(vm_page_reservation* reservation,
	uint32 flags);
struct vm_page *vm_page_allocate_page_run(uint32 flags, page_num_t length,
	const physical_address_restrictions* restrictions, int priority);
struct vm_page *vm_page_at_index(int32 index);
struct vm_page *vm_lookup_page(page_num_t pageNumber);
bool vm_page_is_dummy(struct vm_page *page);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_VM_PAGE_H */
